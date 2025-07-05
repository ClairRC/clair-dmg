#include "ppu.h"
#include "hardware_def.h"
#include "logging.h"
#include "interrupt_handler.h"
#include <stdlib.h>

//Gets value of specific bit (starting at 0)
#define GET_BIT(num, bit) ((num) >> (bit)) & 0x1

//LCDC flag bits
#define BG_WIN_ENABLE 1 << 0
#define OBJ_ENABLE 1 << 1
#define OBJ_SIZE 1 << 2
#define BG_TILE_MAP 1 << 3
#define BG_WIN_INDEX_MODE 1 << 4
#define WIN_ENABLE 1 << 5
#define WIN_TILE_MAP 1 << 6

//Object flag bits
#define OBJ_DMG_PALETTE 1 << 4
#define OBJ_X_FLIP 1 << 5
#define OBJ_Y_FLIP 1 << 6
#define OBJ_PRIORITY 1 << 7

//Color Palette Values
#define PALETTE_WHITE 0xFFFFFF
#define PALETTE_LIGHT_GRAY 0xAAAAAA
#define PALETTE_DARK_GRAY 0x555555
#define PALETTE_BLACK 0x000000

PPU* ppu_init(Memory* mem, DisplayData* display) {
    PPU* ppu = (PPU*)malloc(sizeof(PPU));

    if (ppu == NULL) {
        printError("Unable to initialize PPU.");
        return NULL;
    }

    ppu->memory = mem;

    ppu->state.lcd_on = 0; //Whether LCD is on or not
    ppu->state.window_ly = 0;
    ppu->state.window_ly_increment = 0;
    ppu->state.current_obj_index = 0;
    ppu->state.prev_stat = 0;

    ppu->framebuffer = (uint32_t*)calloc((160 * 144), sizeof(uint32_t)); //Gameboy is 160x144

    if (ppu->framebuffer == NULL) {
        printError("Unable to initialize PPU framebuffer.");
        free(mem);
        free(ppu);
        return NULL;
    }

    //Palette grey-scale colors...
    ppu->dmg_palette[0] = PALETTE_WHITE;
    ppu->dmg_palette[1] = PALETTE_LIGHT_GRAY;
    ppu->dmg_palette[2] = PALETTE_DARK_GRAY;
    ppu->dmg_palette[3] = PALETTE_BLACK;

    ppu->display = display;

    return ppu;
}

void ppu_destroy(PPU* ppu) {
    if (ppu != NULL) {
        if (ppu->memory != NULL)
            free(ppu->memory);

        free(ppu);
    }
}

//Draws frame buffer to screen
void draw(PPU* ppu) {
    draw_buffer(ppu->display, ppu->framebuffer);
}

//Does OAM scan
void ppu_oam_scan(PPU* ppu, uint8_t mode_2_time) {
    //OAM scan takes 80 ticks, but only reads 40 objects
    //This means it takes 2 ticks per object, but for the sake of
    //this emulator, I am just going to do 1 object every other tick

    //Additionally, each scanline can only have 10 objects. On DMG, this is the first 10 suitable objects
    //in OAM, so if we already have 10, then we also don't want to do anything
    if (mode_2_time % 2 != 0 || ppu->state.current_obj_index >= 10)
        return;

    uint8_t obj_height = (ppu->memory->LCDC_LOCATION & OBJ_SIZE) ? 16 : 8; //if bit is set, objects are 8x16, if its clear they're 8x8
    uint8_t obj_offset = (mode_2_time / 2) * 4; //Each attribute is 4 bytes, only once per 2 frames.
    uint16_t obj_base_address = 0xFE00 + obj_offset;

    //Y value is at a 16 pixel offset.. I'm casting this to a 16-bit signed int to avoid underflow
    int16_t obj_y = (int16_t)mem_read(ppu->memory, obj_base_address, PPU_ACCESS); //Byte 0 is y-value
    obj_y = obj_y - 16; //Corresponds to the top-most scanline this object touches

    uint8_t ly = ppu->memory->LY_LOCATION;
    //If object is on scanline, add it to object list.
    if (obj_y <= ly && ly < obj_y + obj_height) {
        store_object(ppu, obj_base_address);
    }
}

//Stores scanline object's adjusted attributes
void store_object(PPU* ppu, uint16_t object_ba) {
    OAM_Entry entry;

    //Y value is at a 16 pixel offset.. I'm casting this to a 16-bit signed int to avoid underflow
    int16_t obj_y = (int16_t)mem_read(ppu->memory, object_ba, PPU_ACCESS); //Byte 0 is y-value
    entry.y_pos = obj_y - 16; //Corresponds to the top-most scanline this object touches

    int16_t obj_x = (int16_t)mem_read(ppu->memory, object_ba + 1, PPU_ACCESS); //Byte 1 is x position
    entry.x_pos = (int16_t)(obj_x)-8; //Similarly to y, the screen x position offset, this time by 8

    entry.tile_index = mem_read(ppu->memory, object_ba + 2, PPU_ACCESS); //Byte 2 is tile index

    entry.flags = mem_read(ppu->memory, object_ba + 3, PPU_ACCESS); //Byte 3 is object flags

    entry.obj_size = (ppu->memory->LCDC_LOCATION & OBJ_SIZE) ? 16 : 8; //if bit is set, objects are 8x16, if its clear they're 8x8

    //If tiles are in 8x16 mode, the upper tile index is at tile_index & 0xFE
    //and the upper tile index isat tile_index | 0x01

    //If we are in 8x16 mode..
    if (entry.obj_size == 16) {
        //If we are in the lower tile...
        if (ppu->memory->LY_LOCATION - obj_y >= 8) {
            entry.tile_index |= 0x01; //Last bit is always 1
            entry.y_pos += 8; //Adjust position for being lower tile
        }
        else {
            entry.tile_index &= 0xFE; //Last bit is always 
        }
    }

    //Finally, add the entry to the Object list for this scanline...
    ppu->scanline_obj[ppu->state.current_obj_index++] = entry;
}

//Writes the color value to the frame buffer
//TODO: Rewrite this whole thing and the helper functions it calls
void ppu_write_lcd(PPU* ppu, uint16_t mode_3_time) {
    uint8_t color_id = get_pixel_color_id(ppu, mode_3_time);

    //Window

    /*uint8_t color_index;
    if (win_is_visible(ppu, mode_3_time)) {
        if (ppu->state.window_ly_increment) {
            ++ppu->state.window_ly;
            ppu->state.window_ly_increment = 0;
        }

        color_index = get_win_color_index(ppu, mode_3_time);
    }
    else
        color_index = 1; 


    //uint8_t color_id = (ppu->memory->BGP_LOCATION >> (2 * color_index)) & 0x3;*/

    //Get framebuffer index and ouput to frame buffer
    uint32_t framebuffer_i = 160 * ppu->memory->LY_LOCATION + mode_3_time;
    ppu->framebuffer[framebuffer_i] = ppu->dmg_palette[color_id];
}

//Gets the color id that belongs at the current pixel
uint8_t get_pixel_color_id(PPU* ppu, uint16_t mode_3_time) {
    int obj_i = visible_obj_index(ppu, mode_3_time); //Gets object index
    uint8_t window_color_index = 0xFF;
    uint8_t bg_color_index = get_bg_color_index(ppu, mode_3_time); //BG color

    //255 if no window, window color otherwise
    if (win_is_visible(ppu, mode_3_time)) {
        //Increment internal window counter if it hasnt been
        if (ppu->state.window_ly_increment) {
            ++ppu->state.window_ly;
            ppu->state.window_ly_increment = 0;
        }
        window_color_index = get_win_color_index(ppu, mode_3_time);
    }

    //Gets priority between window and background
    uint8_t non_obj_color_index = (window_color_index == 0xFF) ? bg_color_index : window_color_index;

    //If there's an object at this location, its index is not 0, and priority bit is 0 or is 1 and win/bg index is 0, draw object
    //Priority for this is a little confusing but it'll get more confusing when I implement CGB

    //If there IS an object on this scanline...
    if (obj_i >= 0) {
        uint8_t obj_color_index = get_obj_color_index(ppu, mode_3_time, obj_i);
        uint8_t obj_flags = ppu->scanline_obj[obj_i].flags; //Object flags

        //Index 0 means transparent, so only do this if its not 0
        if (obj_color_index != 0) {
            //If priority bit is clear, object always has highest priority
            //If it is set, it only gets priority over bg/win index 0
            if (!(obj_flags & OBJ_PRIORITY) || non_obj_color_index == 0)
                return obj_color_id_from_index(ppu, obj_color_index, obj_flags & OBJ_DMG_PALETTE);
        }
    }

    uint8_t bg_win_color_id = (ppu->memory->BGP_LOCATION >> (2 * non_obj_color_index)) & 0x3; //Gets 2 bit color ID from the palette

    return bg_win_color_id;
}

//Gets the object's color ID from its color index
uint8_t obj_color_id_from_index(PPU* ppu, uint8_t color_index, uint8_t palette) {
    uint8_t obj_palette = palette ? ppu->memory->OBP1_LOCATION : ppu->memory->OBP0_LOCATION; //Palette value based on palette flag
    uint8_t color_id = (obj_palette >> (2 * color_index)) & 0x3; //Gets 2 bit color ID from the palette

    return color_id;
}

//Gets the index of the current background pixel tile
uint8_t get_bg_color_index(PPU* ppu, uint16_t mode_3_time) {
    //Get LY and LCDC values
    uint8_t ly = ppu->memory->LY_LOCATION; //LY register
    uint8_t lcdc = ppu->memory->LCDC_LOCATION; //LCDC register

    //If BG/Window are disabled, return white instead
    if (!(lcdc & BG_WIN_ENABLE))
        return 0;

    //Top-left coordinates of the pixels based on the scroll registers
    //This wraps at 256
    uint8_t scroll_x = ppu->memory->SCX_LOCATION;
    uint8_t scroll_y = ppu->memory->SCY_LOCATION;

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
    uint8_t tile_index = mem_read(ppu->memory, tile_address, PPU_ACCESS);

    //Gets color index for the tile index
    uint8_t color_index = get_color_index(ppu, tile_pixel_x, tile_pixel_y, tile_index, lcdc & BG_WIN_INDEX_MODE); //Color id!! (0-3)

    return color_index;
}

//Gets index of window color if there is one
uint8_t get_win_color_index(PPU* ppu, uint16_t mode_3_time) {
    //Get useful values
    uint8_t ly = ppu->memory->LY_LOCATION; //LY register
    uint8_t lcdc = ppu->memory->LCDC_LOCATION; //LCDC register
    uint16_t wx = ppu->memory->WX_LOCATION - 7; //Adjusted window scroll
    uint16_t wy = ppu->memory->WY_LOCATION;

    //If BG/Window are disabled, return white instead
    if (!(lcdc & BG_WIN_ENABLE))
        return 0;

    //Tilemap x/y coordinates...
    uint8_t x = mode_3_time - wx;
    uint8_t y = ppu->state.window_ly - 1;

    uint16_t tilemap_x = x / 8;
    uint16_t tilemap_y = y / 8;

    uint16_t tile_pixel_x = x % 8;
    uint16_t tile_pixel_y = y % 8;

    //Find tilemap base address
    uint16_t tilemap_base = (lcdc & WIN_TILE_MAP) ? 0x9C00 : 0x9800; //Base address of the tilemap
    uint16_t tile_address = tilemap_base + (32 * tilemap_y) + tilemap_x; //Get address of the tile in the tilemap

    //Gets tile data index
    uint8_t tile_index = mem_read(ppu->memory, tile_address, PPU_ACCESS);

    //Gets color index for the tile index
    uint8_t color_index = get_color_index(ppu, tile_pixel_x, tile_pixel_y, tile_index, lcdc & BG_WIN_INDEX_MODE); //Color index!! (0-3)

    return color_index;
}

//Gets object color. An 0x01 indicates a clear color
uint8_t get_obj_color_index(PPU* ppu, uint16_t mode_3_time, int object_index) {
    //Get useful values
    uint8_t ly = ppu->memory->LY_LOCATION; //LY register
    uint8_t lcdc = ppu->memory->LCDC_LOCATION; //LCDC register

    //If no object is visible, return 0, which means don't draw object
    if (object_index == -1)
        return 0;

    OAM_Entry obj = ppu->scanline_obj[object_index];

    //Account for flip
    uint8_t tile_x = mode_3_time - obj.x_pos;
    uint8_t tile_y = ly - obj.y_pos;
    uint8_t tile_index = ppu->scanline_obj[object_index].tile_index;

    //X flip
    //Get ther other 
    if (obj.flags & OBJ_X_FLIP)
        tile_x = 7 - tile_x;

    //Y flip
    if (obj.flags & OBJ_Y_FLIP) {
        tile_y = 7 - tile_y;

        //If object is 8x16, also flip the tiles
        if (ppu->scanline_obj[object_index].obj_size == 16) {
            if (tile_index & 0x1) {
                tile_index &= 0xFE;
            }
            else {
                tile_index |= 0x1;
            }
        }
    }

    //Object always uses tilemap area 1 and 0x8000 indexing mode
    uint8_t color_index = get_color_index(ppu, tile_x, tile_y, tile_index, 1);

    return color_index;
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
    uint8_t tile_color_lsb = mem_read(ppu->memory, tile_data_address, PPU_ACCESS);
    uint8_t tile_color_msb = mem_read(ppu->memory, tile_data_address + 1, PPU_ACCESS);

    uint8_t tile_color_lower_bit = GET_BIT(tile_color_lsb, 7 - tile_x); //Gets lower bit
    uint8_t tile_color_upper_bit = GET_BIT(tile_color_msb, 7 - tile_x); //Gets upper bit

    uint8_t color_id = (tile_color_upper_bit << 1) | tile_color_lower_bit; //Color index!! (0-3)

    return color_id;
}

//Returns 1 for visible, 0 for not visible on this pixel
uint8_t win_is_visible(PPU* ppu, uint16_t mode_3_time) {
    uint8_t lcdc = ppu->memory->LCDC_LOCATION;
    uint16_t wx = ppu->memory->WX_LOCATION - 7;
    uint16_t wy = ppu->memory->WY_LOCATION;
    uint16_t ly = ppu->memory->LY_LOCATION;

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
    uint8_t lcdc = ppu->memory->LCDC_LOCATION;
    uint16_t ly = ppu->memory->LY_LOCATION;

    //Object disabled means no object
    if (!(lcdc & OBJ_ENABLE))
        return -1;

    int index = -1;

    //Looks through objects on the scanline to find one at this location
    for (int i = 0; i < ppu->state.current_obj_index; ++i) {
        if ((mode_3_time >= ppu->scanline_obj[i].x_pos && mode_3_time < ppu->scanline_obj[i].x_pos + 8)
            && (ly >= ppu->scanline_obj[i].y_pos && ly < ppu->scanline_obj[i].y_pos + 8)) {

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

//Switch from mode 0 to mode 1 (hblank to vblank)
void switch_mode_0_1(PPU* ppu) {
    //If we just started vblank, request interrupt now
    requestInterrupt(INTERRUPT_VBLANK, ppu->memory);
    
    ppu->state.window_ly = 0; //Reset internal window counter
    ppu->memory->state.current_ppu_mode = PPU_MODE_1; //Update PPU mode
}

//Switch from mode 1 to mode 2 (vblank to oam scan)
void switch_mode_1_2(PPU* ppu) {
    //At end of VBLANK
    ppu->memory->state.current_ppu_mode = PPU_MODE_2; //Update current PPU mode

    draw(ppu); //Draw frame
}

//Switch from mode 2 to mode 3 (oam scan to draw scanline)
void switch_mode_2_3(PPU* ppu) {
    ppu->state.window_ly_increment = 1; //Allow window LY to get incremented again for this scanline
    ppu->memory->state.current_ppu_mode = PPU_MODE_3; //Update PPU mode to mode 3
}

//Switch from mode 3 to mode 0 (draw scanline to hblank)
void switch_mode_3_0(PPU* ppu) {
    ppu->state.current_obj_index = 0; //Reset scanline sprite index
    ppu->memory->state.current_ppu_mode = PPU_MODE_0;
}

//Switch from mode 0 to mode 2 (hblank to oam scan)
void switch_mode_0_2(PPU* ppu) {
    ppu->memory->state.current_ppu_mode = PPU_MODE_2;
}