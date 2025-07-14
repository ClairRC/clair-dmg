#include "ppu.h"

/*
* This file holds the definitions for PPU helper functions.
* The inital file was getting too big and difficult to maintain.
* The PPU has a lot of weird quirks..
*/

#define GET_BIT(num, bit) ((num) >> (bit)) & 0x1 //Gets value of specific bit (starting at 0)

//Updates PPU mode
void update_ppu_state(PPU* ppu) {
    uint32_t frame_time = ppu->global_state->frame_time;

    //If PPU is currently on, update LY
    if (ppu->global_state->lcd_on)
        ppu->bus->memory->LY_LOCATION = frame_time / SCANLINE_END; //Update LY

    //If PPU is off, mode is Off, keep LY at 0
    else {
        ppu->global_state->current_mode = PPU_MODE_OFF;
        ppu->bus->memory->LY_LOCATION = 0;

        return; //No updates are done if PPU is off
    }

    //If vblank ends and a new frame starts...
    if (frame_time == MODE_1_END)
        switch_mode_1_2(ppu);

    //Only update the state if we are not in vblank

    uint16_t ly = ppu->bus->memory->LY_LOCATION;
    //At the end of mode 2
    if (frame_time % SCANLINE_END == MODE_2_END && ppu->global_state->current_mode != PPU_MODE_1)
        switch_mode_2_3(ppu);

    //At the end of mode 3
    if (frame_time % SCANLINE_END == MODE_3_END && ppu->global_state->current_mode != PPU_MODE_1)
        switch_mode_3_0(ppu);

    //At end of mode 0
    if (frame_time % SCANLINE_END == MODE_0_END && ppu->global_state->current_mode != PPU_MODE_1) {
        //If vblank is beginning, switch to vblank
        if (frame_time == VBLANK_BEGIN)
            switch_mode_0_1(ppu);

        //Otherwise, switch back to OAM scan
        else
            switch_mode_0_2(ppu);
    }
}

//Stores scanline object's adjusted attributes
void store_object(PPU* ppu, uint16_t object_ba) {
    OAM_Entry entry;

    //Y value is at a 16 pixel offset.. I'm casting this to a 16-bit signed int to avoid underflow
    int16_t obj_y = (int16_t)mem_read(ppu->bus, object_ba, PPU_ACCESS); //Byte 0 is y-value
    entry.y_pos = obj_y - 16; //Corresponds to the top-most scanline this object touches

    int16_t obj_x = (int16_t)mem_read(ppu->bus, object_ba + 1, PPU_ACCESS); //Byte 1 is x position
    entry.x_pos = (int16_t)(obj_x)-8; //Similarly to y, the screen x position offset, this time by 8

    entry.tile_index = mem_read(ppu->bus, object_ba + 2, PPU_ACCESS); //Byte 2 is tile index

    entry.flags = mem_read(ppu->bus, object_ba + 3, PPU_ACCESS); //Byte 3 is object flags

    //Finally, add the entry to the Object list for this scanline...
    ppu->scanline_obj[ppu->local_state.current_obj_index++] = entry;
}

//Gets the color id that belongs at the current pixel
uint8_t get_pixel_color_id(PPU* ppu, uint16_t mode_3_time) {
    ObjColorData obj_color_data = get_obj_color_data(ppu, mode_3_time); //Gets object color index and flags
    uint8_t window_color_index = 0xFF;
    uint8_t bg_color_index = get_bg_color_index(ppu, mode_3_time); //BG color

    //255 if no window, window color otherwise
    if (win_is_visible(ppu, mode_3_time)) {
        //Increment internal window counter if it hasnt been
        if (ppu->local_state.window_ly_increment) {
            ++ppu->local_state.window_ly;
            ppu->local_state.window_ly_increment = 0;
        }
        window_color_index = get_win_color_index(ppu, mode_3_time);
    }

    //Gets priority between window and background
    uint8_t non_obj_color_index = (window_color_index == 0xFF) ? bg_color_index : window_color_index;

    //If an object is getting drawn
    if (obj_color_data.color != 0) {
        //If no background/window priority
        if (!(obj_color_data.flags & OBJ_PRIORITY) || non_obj_color_index == 0)
            //Return object color 
            return obj_color_id_from_index(ppu, obj_color_data.color, obj_color_data.flags & OBJ_DMG_PALETTE);
    }

    uint8_t bg_win_color_id;

    //If BG/Window are disabled, draw white
    //TODO: I don't think this is necessarily correct because this might not correspond to white depending on palette
    if (!(ppu->bus->memory->LCDC_LOCATION & BG_WIN_ENABLE))
        bg_win_color_id = 0;

    //Otherwise, return non-object color
    else
        bg_win_color_id = (ppu->bus->memory->BGP_LOCATION >> (2 * non_obj_color_index)) & 0x3; //Gets 2 bit color ID from the palette

    return bg_win_color_id;
}

//Gets the object's color ID from its color index
uint8_t obj_color_id_from_index(PPU* ppu, uint8_t color_index, uint8_t palette) {
    uint8_t obj_palette = palette ? ppu->bus->memory->OBP1_LOCATION : ppu->bus->memory->OBP0_LOCATION; //Palette value based on palette flag
    uint8_t color_id = (obj_palette >> (2 * color_index)) & 0x3; //Gets 2 bit color ID from the palette

    return color_id;
}

//Gets the index of the current background pixel tile
uint8_t get_bg_color_index(PPU* ppu, uint16_t mode_3_time) {
    //Get LY and LCDC values
    uint8_t ly = ppu->bus->memory->LY_LOCATION; //LY register
    uint8_t lcdc = ppu->bus->memory->LCDC_LOCATION; //LCDC register

    //Top-left coordinates of the pixels based on the scroll registers
    //This wraps at 256
    uint8_t scroll_x = ppu->bus->memory->SCX_LOCATION;
    uint8_t scroll_y = ppu->bus->memory->SCY_LOCATION;

    //Account for wrapping arround
    //Pixel coordinates on the tilemap
    uint8_t x = mode_3_time + scroll_x;
    uint8_t y = ly + scroll_y;

    uint16_t tilemap_x = x / 8;
    uint16_t tilemap_y = y / 8;

    uint16_t tile_pixel_x = x % 8;
    uint16_t tile_pixel_y = y % 8;

    //Find tilemap base address
    uint16_t tilemap_base = (lcdc & BG_TILE_MAP) ? 0x9C00 : 0x9800; //Base address of the tilemap
    uint16_t tile_address = tilemap_base + (32 * tilemap_y) + tilemap_x; //Get address of the tile in the tilemap

    //Gets tile data index
    uint8_t tile_index = mem_read(ppu->bus, tile_address, PPU_ACCESS);

    //Gets color index for the tile index
    uint8_t color_index = get_color_index(ppu, tile_pixel_x, tile_pixel_y, tile_index, lcdc & BG_WIN_INDEX_MODE); //Color id!! (0-3)

    return color_index;
}

//Gets index of window color if there is one
uint8_t get_win_color_index(PPU* ppu, uint16_t mode_3_time) {
    //Get useful values
    uint8_t ly = ppu->bus->memory->LY_LOCATION; //LY register
    uint8_t lcdc = ppu->bus->memory->LCDC_LOCATION; //LCDC register
    int16_t wx = ppu->bus->memory->WX_LOCATION - 7; //Adjusted window scroll
    int16_t wy = ppu->bus->memory->WY_LOCATION;

    //Tilemap x/y coordinates...
    uint8_t x = mode_3_time - wx;
    uint8_t y = ppu->local_state.window_ly - 1;

    uint16_t tilemap_x = x / 8;
    uint16_t tilemap_y = y / 8;

    uint16_t tile_pixel_x = x % 8;
    uint16_t tile_pixel_y = y % 8;

    //Find tilemap base address
    uint16_t tilemap_base = (lcdc & WIN_TILE_MAP) ? 0x9C00 : 0x9800; //Base address of the tilemap
    uint16_t tile_address = tilemap_base + (32 * tilemap_y) + tilemap_x; //Get address of the tile in the tilemap

    //Gets tile data index
    uint8_t tile_index = mem_read(ppu->bus, tile_address, PPU_ACCESS);

    //Gets color index for the tile index
    uint8_t color_index = get_color_index(ppu, tile_pixel_x, tile_pixel_y, tile_index, lcdc & BG_WIN_INDEX_MODE); //Color index!! (0-3)

    return color_index;
}

//Gets object color. 0 indicates no object should be drawn
ObjColorData get_obj_color_data(PPU* ppu, uint16_t mode_3_time) {
    //Get useful values
    uint8_t ly = ppu->bus->memory->LY_LOCATION; //LY register
    uint8_t lcdc = ppu->bus->memory->LCDC_LOCATION; //LCDC register

    ObjColorData dat = (ObjColorData){ .color = 0, .flags = 0 };

    //Object disabled means no object
    if (!(lcdc & OBJ_ENABLE))
        return dat;


    //Find object index to draw on scanline 
    uint8_t lowest_x = 255;

    for (int i = 0; i < ppu->local_state.current_obj_index; ++i) {
        OAM_Entry obj = ppu->scanline_obj[i];

        //If there is no object at this pixel, skip the check
        if (!((mode_3_time >= obj.x_pos && mode_3_time < obj.x_pos + 8)))
            continue;

        uint8_t tile_x = mode_3_time - obj.x_pos;
        uint8_t tile_y = ly - obj.y_pos;
        uint8_t tile_index = ppu->scanline_obj[i].tile_index;

        if (ppu->bus->memory->LCDC_LOCATION & OBJ_SIZE) {
            //For 8x16 objects, bit 0 is ignored for top tile but set for bottom tile
            if (tile_y >= 8) {
                tile_index |= 0x01;
                tile_y -= 8;
            }
            else {
                tile_index &= 0xFE;
            }
        }

        //Account for flip

        //X flip
        //Get ther other 
        if (obj.flags & OBJ_X_FLIP)
            tile_x = 7 - tile_x;

        //Y flip
        if (obj.flags & OBJ_Y_FLIP) {
            tile_y = 7 - tile_y;

            //Flip tiles if 8x16
            if (ppu->bus->memory->LCDC_LOCATION & OBJ_SIZE) {
                if ((tile_index & 0x1) == 1) {
                    tile_index &= 0xFE;
                }

                else
                    tile_index |= 0x1;
            }
        }

        //If object has the lowest x position at this pixel AND is not 0, update the color index
        uint8_t new_color = get_color_index(ppu, tile_x, tile_y, tile_index, 1);

        //If x pos lowe than lowest x and the color isn't 0, update color to draw
        if (obj.x_pos < lowest_x && new_color != 0) {
            dat.color = new_color;
            dat.flags = obj.flags;
            lowest_x = obj.x_pos;
        }
    }

    return dat;
}

uint8_t get_color_index(PPU* ppu, uint8_t tile_x, uint8_t tile_y, uint8_t tile_index, uint8_t tile_area) {
    uint16_t tile_data_base_address;

    //Find the indexing mode...
    //If bit is clear, add signed value to 0x9000
    //If bit is set, add unisnged value to 0x8000
    if (tile_area)
        tile_data_base_address = 0x8000 + (tile_index * 16);
    else
        tile_data_base_address = 0x9000 + (((int8_t)tile_index) * 16); //Signed access to 0x8800-0x97FF

    //Get which byte to read from the y-pixel...
    //y=0 -> byte 0 and 1, y=1 -> byte 2 and 3....
    uint16_t tile_data_address = (2 * tile_y) + tile_data_base_address;

    //FINALLY read the bytes for these pixels
    uint8_t tile_color_lsb = mem_read(ppu->bus, tile_data_address, PPU_ACCESS);
    uint8_t tile_color_msb = mem_read(ppu->bus, tile_data_address + 1, PPU_ACCESS);

    uint8_t tile_color_lower_bit = GET_BIT(tile_color_lsb, 7 - tile_x); //Gets lower bit
    uint8_t tile_color_upper_bit = GET_BIT(tile_color_msb, 7 - tile_x); //Gets upper bit

    uint8_t color_id = (tile_color_upper_bit << 1) | tile_color_lower_bit; //Color index!! (0-3)

    return color_id;
}

//Returns 1 for visible, 0 for not visible on this pixel
uint8_t win_is_visible(PPU* ppu, uint16_t mode_3_time) {
    uint8_t lcdc = ppu->bus->memory->LCDC_LOCATION;
    int16_t wx = ppu->bus->memory->WX_LOCATION - 7;
    int16_t wy = ppu->bus->memory->WY_LOCATION;
    uint16_t ly = ppu->bus->memory->LY_LOCATION;

    //If window is not enabled, return 0
    if (!(lcdc & WIN_ENABLE))
        return 0;

    //If window hasn't appeared on screen yet, its not visible
    if (wx > mode_3_time || wy > ly)
        return 0;

    //If window has already finished rendering, It's not visible
    if (wx + 160 <= mode_3_time || wy + 144 <= ly)
        return 0;

    return 1;
}

//Returns index of object visible at this pixel
//Returns -1 if there is no visible object
int visible_obj_index(PPU* ppu, uint16_t mode_3_time) {
    uint8_t lcdc = ppu->bus->memory->LCDC_LOCATION;
    uint16_t ly = ppu->bus->memory->LY_LOCATION;

    //Object disabled means no object
    if (!(lcdc & OBJ_ENABLE))
        return -1;

    int index = -1;

    //Looks through objects on the scanline to find one at this location
    for (int i = 0; i < ppu->local_state.current_obj_index; ++i) {
        if ((mode_3_time >= ppu->scanline_obj[i].x_pos && mode_3_time < ppu->scanline_obj[i].x_pos + 8)) {

            //Lowest x position object has priority
            if (index != -1) {
                if (ppu->scanline_obj[i].x_pos < ppu->scanline_obj[index].x_pos)
                    index = i;
            }
            else
                index = i;
        }
    }

    return index;
}