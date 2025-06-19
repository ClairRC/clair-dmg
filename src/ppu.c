#include "ppu.h"
#include "hardware_def.h"
#include "logging.h"
#include "interrupt_handler.h"
#include <stdlib.h>

PPU* ppu_init(Memory* mem) {
    PPU* ppu = (PPU*)malloc(sizeof(PPU));

    if (ppu == NULL) {
        printError("Unable to initialize PPU.");
        return NULL;
    }

    ppu->memory = mem;
    ppu->frame_time = 0;
    ppu->current_oam_index = 0;

    //Place holder...
    ppu->palette[0] = '\'';
    ppu->palette[1] = '~';
    ppu->palette[2] = '%';
    ppu->palette[3] = '#';

    return ppu;
}

void ppu_destroy(PPU* ppu) {
    if (ppu != NULL) {
        if (ppu->memory != NULL)
            free(ppu->memory);

        free(ppu);
    }
}

//Updates PPU given a number of cycles
void update_ppu(PPU * ppu, uint16_t time) {
    //For each tick..
    for (int i = 0; i < time; ++i) {
        update_ppu_mode(ppu); //updates current ppu mode

        if (ppu->memory->current_ppu_mode == PPU_MODE_2)
            ppu_oam_scan(ppu, ppu->frame_time % 80);

        if (ppu->memory->current_ppu_mode == PPU_MODE_3)
            ppu_write_lcd(ppu,ppu->frame_time % 172);

        //Update frame timer..
        ++ppu->frame_time;

        if (ppu->frame_time >= 72204) {
            ppu->frame_time = 0;
            //ppu->memory->current_ppu_mode = PPU_MODE_3;
            ppu->vblank_active = 0;
            draw(ppu);
        }
    }
}

#include <stdio.h>
void draw(PPU* ppu) {
    for (int i = 0; i < 144; ++i) {
        for (int j = 0; j < 160; j++) {
            printf("%c", ppu->palette[ppu->framebuffer[(i * 160) + j]]);
        }
        printf("\n");
    }

    printf("\n\n");
}

//Does the OAM scan part of the PPU updating
void ppu_oam_scan(PPU* ppu, uint16_t tick) {
    //This checks 1 OAM index per 2 ticks, so I'm just going to simplify
    //this by only doing this on factors of 2.
    //Also if we already have 10 objects, we're done too
    if (tick % 2 != 0 || ppu->current_oam_index == 10)
        return; 

    uint8_t current_scanline = ppu->memory->io[0x44]; //Gets the current scanline
    

    //As far as I know, this just gets a list of the objects that are on this scanline and the order to draw them...
    //Each object is 4 bytes
    uint8_t memory_oam_index = tick / 2; //Which object we're checking
    uint8_t* object_base = &ppu->memory->oam[4 * memory_oam_index];
    uint8_t object_location = object_base[0] - 16; //This byte is its location + 16
    uint8_t object_height = 8; //Objects are 8 pixels tall by defailt

    //If LCDC bit 2 is 1, objects are 16 tall
    if (ppu->memory->io[0x40] & 0x4)
        object_height = 16;

    //If this object is On this scanline, add it to the oam array...
    if (current_scanline >= object_location && current_scanline < object_location + object_height) {
        uint8_t tile_index; //Index for the object tile data

        //If PPU is in 8x16 mode...
        if (object_height == 16) {
            //If this is the bottom half of the sprite
            if (current_scanline - object_location >= 8)
                //This byte only specifies the index of the top tile, the bottom is right after
                tile_index = object_base[2] | 0x01;
            else
                tile_index = object_base[2] & 0xFE; //Otherwise get the one that Isn't the bottom half

        }

        //In 8x8 mode, tile index is just there
        else
            tile_index = object_base[2]; 

        //Add OAM entry
        ppu->oam_indices[ppu->current_oam_index++] = (OAM_Entry)
            {.y_pos = object_base[0], .x_pos = object_base[1],
            .tile_index = tile_index, .flags = object_base[3]};
    }
}

//Writes the color value to the frame buffer
void ppu_write_lcd(PPU* ppu, uint16_t tick) {
    if (tick >= 160) {
        return; 
    }
    
    //Gets pixel value and writes it to frame buffer till its full.
    //Bottom right coordinates of the view port based on the scroll registers...
    //This loops, so that's why its uint8_t...

    //Tilemaps are 256x256 pixels, so the scroll just says what the viewport is...
    uint8_t left_coordinate = ppu->memory->io[0x43];
    uint8_t top_coordinate = ppu->memory->io[0x42];
    uint8_t scanline = ppu->memory->io[0x44];
    
    //Current pixel we're writing to
    //The scanline is the screen y-coordinate and tick is the x...
    int framebuffer_i = (scanline * 160) + tick;
    
    //Tilemap coordinates
    //This is the coordinates of the full grid as well as their indicies in the
    //VRAM tile map....
    uint8_t tilemap_x = left_coordinate + tick;
    uint8_t tilemap_y = top_coordinate + scanline;
    uint8_t tilemap_index_x = tilemap_x/8;
    uint8_t tilemap_index_y = tilemap_y/8;

    //Which pixel of the tile is getting drawn
    uint8_t pixel_x = tilemap_x % 8;
    uint8_t pixel_y = tilemap_y % 8;
    
    //LCDC (we will need this a lot)
    uint8_t lcdc = ppu->memory->io[0x40];

    //This is going to be somewhat inaccurate to real behavior, because the actual drawing
    //does not need to be too accurate as long as the frame buffer is pushed at the end of the frame..
    //So I will just draw background, then window if its active, then object (or object before window sometimes)
    uint8_t bg_tilemap = (lcdc >> 3) & 0x01; //Bit 3 is tilemap
    uint8_t index_mode = (lcdc >> 4) & 0x01; //Bit 4 is indexing mode
    uint8_t window_enable = (lcdc >> 5) & 0x01; //Bit 5 is window enable
    uint8_t window_tilemap = (lcdc >> 6) & 0x01; //Bit 6 is window tilemap

    //Gets the correct tile and pixel for each layer...
    uint16_t tilemap_offset = bg_tilemap ? 0x9C00 : 0x9800; //Which tilemap is being accessed
    
    //Gets tile data address for BG..
    uint16_t tile_address = tilemap_offset + ((tilemap_index_y * 32) + tilemap_index_x);
    uint16_t tile_data_index = mem_read(ppu->memory, tile_address, PPU_ACCESS);
    uint16_t tile_data_base_address;

    //Get the actual base address for the tile color data..........
    if (index_mode)
        tile_data_base_address = 0x8000 + tile_data_index;
    else
        tile_data_base_address = 0x9000 + (int8_t)tile_data_index;

    //FINALLY THE PIXEL IS!!!
    //The bytes we need are... 2y and 2y+1. I'll comment better later...
    uint8_t least_significant_color_bit = mem_read(ppu->memory, tile_data_base_address + (2*pixel_y), PPU_ACCESS);
    uint8_t most_significant_color_bit = mem_read(ppu->memory, tile_data_base_address + (2*pixel_y) + 1, PPU_ACCESS);

    least_significant_color_bit = (least_significant_color_bit >> (7 - pixel_x))  & 0x01;
    most_significant_color_bit = (most_significant_color_bit >> (7 - pixel_x))  & 0x01;
    int color_index = most_significant_color_bit << 1 | least_significant_color_bit;

    ppu->framebuffer[framebuffer_i] = color_index;
}

//Updates PPU mode the number of ticks has passed is correct
void update_ppu_mode(PPU* ppu) {
    //Every scanline, which is 456 ticks, the following happens:
    //At tick 0, OAM scan starts (mode 2)
    if (ppu->frame_time % 456 == 0) {
        ++ppu->memory->io[0x44];
        
        //If LY is 144 or higher, vblank is active
        if (ppu->frame_time == 65664) {
            //Set the flag and request vblank interrupt
            ppu->vblank_active = 1;
            ppu->memory->current_ppu_mode = PPU_MODE_1;
            requestInterrupt(INTERRUPT_VBLANK, ppu->memory);
        }

        //If LY is less than 144, then start mode 2 as normal
        else {
            ppu->current_oam_index = 0; //Reset OAM index for OAM scan
            ppu->memory->current_ppu_mode = PPU_MODE_2;
        }
    }

    //At tick 80, pixel rendering begins
    if (ppu->frame_time % 456 == 80 && !ppu->vblank_active)
        ppu->memory->current_ppu_mode = PPU_MODE_3;

    //At tick 252 (roughly), Hblank starts
    //Technically this CAN last less or more time, but for this emulator's sake,
    //I am going with general timing
    if (ppu->frame_time % 456 == 252 && !ppu->vblank_active)
        ppu->memory->current_ppu_mode = PPU_MODE_0;

    //TODO: Implement end of frame behavior
}