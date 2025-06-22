#include "ppu.h"
#include "hardware_def.h"
#include "logging.h"
#include "interrupt_handler.h"
#include <stdlib.h>

//Gets value of specific bit (starting at 0)
#define GET_BIT(num, bit) ((num) >> (bit)) & 0x1

//LCDC bit meanings. LCDC gets checed a lot...
#define BG_WIN_ENABLE 1 << 0
#define OBJ_ENABLE 1 << 1
#define OBJ_SIZE 1 << 2
#define BG_TILE_MAP 1 << 3
#define BG_WIN_INDEX_MODE 1 << 4
#define WIN_ENABLE 1 << 5
#define WIN_TILE_MAP 1 << 6
#define LCD_ENABLE 1 << 7

PPU* ppu_init(Memory* mem) {
    PPU* ppu = (PPU*)malloc(sizeof(PPU));

    if (ppu == NULL) {
        printError("Unable to initialize PPU.");
        return NULL;
    }

    ppu->memory = mem;
    ppu->frame_time = 0;
    ppu->current_oam_index = 0;
    ppu->framebuffer_i = 0;
    ppu->prev_stat = 0;

    //Place holder...
    ppu->palette[0] = ' \'';
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
void update_ppu(PPU* ppu, uint16_t time) {
    uint16_t elapsed_ticks = 0; //Ticks elapsed this updated
    uint16_t scanline_time = ppu->frame_time % 456; //Current scanline time... Mostly for testing right now

    //Do whatever the current mode is, and after it's done, update PPU state...
    //If state changes it might enter more than one of these if statements.

    //Do these all until we've used all the ticks...
    while (elapsed_ticks < time) {

        //If LCD is off..
        if (ppu->memory->current_ppu_mode == PPU_MODE_OFF) {
            while (elapsed_ticks < time && !(ppu->memory->io[0x40] >> 7))
                ++elapsed_ticks;

            update_ppu_state(ppu, elapsed_ticks);
        }

        //If in OAM scan
        if (ppu->memory->current_ppu_mode == PPU_MODE_2) {
            //TODO: Implement OAM scan
            while (elapsed_ticks < time && scanline_time < 80) {
                ++elapsed_ticks;
                ++scanline_time;
            }

            update_ppu_state(ppu, elapsed_ticks);
        }

        //If drawing pixels
        if (ppu->memory->current_ppu_mode == PPU_MODE_3) {
            elapsed_ticks += ppu_write_lcd(ppu, time);

            update_ppu_state(ppu, elapsed_ticks);
        }

        //If in hblank..
        if (ppu->memory->current_ppu_mode == PPU_MODE_0) {
            while (elapsed_ticks < time && scanline_time < 456) { //Just wait
                ++elapsed_ticks;
                ++scanline_time;
            }

            update_ppu_state(ppu, elapsed_ticks);
        }

        //If we are in vblank
        if (ppu->memory->current_ppu_mode == PPU_MODE_1) {
            while (elapsed_ticks < time && ppu->frame_time < 70224) {
                ++elapsed_ticks;
                //If a new scanline starts, update state to update LY and such
                update_ppu_state(ppu, elapsed_ticks);
            }
        }
    }
}


#include <stdio.h>
void draw(PPU* ppu) {

}

//Does the OAM scan part of the PPU updating
void ppu_oam_scan(PPU* ppu, uint16_t tick) {
    
}

//Writes the color value to the frame buffer
int ppu_write_lcd(PPU* ppu, uint16_t tick) {
    return write_bg_win(ppu, tick);
}

//Updates PPU state including LY, mode, etc...
void update_ppu_state(PPU* ppu, uint16_t ticks) {
    ppu->frame_time += ticks;

    //If vblank ends and a new frame starts...
    if (ppu->frame_time >= 70224) {
        //Reset mode back to OAM scan, draw frame, reset frame buffer and OAM
        ppu->frame_time -= 70224;
        ppu->memory->current_ppu_mode = PPU_MODE_2;
        ppu->current_oam_index = 0;
        ppu->framebuffer_i = 0;
        ppu->memory->io[0x44] = 0; //Reset scanline to 0

        draw(ppu);
    }

    //Updates current scanline time and LY value
    uint16_t scanline_time = ppu->frame_time % 456; //Time in this scanline so far
    ppu->memory->io[0x44] = ppu->frame_time / 456; //Sets current LY

    //Sets previous PPU mode to be whatever it was at the start of the update for STAT interrupt purposes...
    //TODO: This logic can probably be better?
    ppu->memory->previous_ppu_mode = ppu->memory->current_ppu_mode;

    if (ppu->memory->io[0x44] >= 144) {
        //If we jsut started vblank, request interrupt now
        if (ppu->memory->current_ppu_mode == PPU_MODE_0) {
            //printf("vblank_interrupt current tick:%d ly:%d\n", ppu->frame_time, ppu->memory->io[0x44]);
            requestInterrupt(INTERRUPT_VBLANK, ppu->memory);
        }
        
        //printf("ly:%d mode1 scanline_tick:%d\n", ppu->memory->io[0x44], scanline_time);
        ppu->memory->current_ppu_mode = PPU_MODE_1;
    }

    else if (scanline_time < 80) {
        //printf("ly:%d mode2 scanline_tick:%d\n", ppu->memory->io[0x44], scanline_time);
        ppu->memory->current_ppu_mode = PPU_MODE_2;
    }

    else if (scanline_time < 252) {
        //printf("ly:%d mode3 scanline_tick:%d\n", ppu->memory->io[0x44], scanline_time);
        ppu->memory->current_ppu_mode = PPU_MODE_3;
    }

    else if (scanline_time < 456) {
        //printf("ly:%d mode0 scanline_tick:%d\n", ppu->memory->io[0x44], scanline_time);
        ppu->memory->current_ppu_mode = PPU_MODE_0;
    }

    //If LCDC.7 is 0, PPU is actually off and reset these thingss
    if (!(ppu->memory->io[0x40] >> 7)) {
        ppu->memory->current_ppu_mode = PPU_MODE_OFF;
        ppu->frame_time = 0;
        ppu->memory->io[0x44] = 0;
    }

    //Update LCD Status
    update_stat(ppu);

    //printf("Mode:%d Time:%d\n",ppu->memory->current_ppu_mode, ppu->frame_time%456);

}

void update_stat(PPU* ppu) {
    //LYC is if LYC register and LY are equal
    uint8_t lyc = mem_read(ppu->memory, 0xFF44, PPU_ACCESS) == mem_read(ppu->memory, 0xFF45, PPU_ACCESS);
    uint8_t current_stat = 0; //Current interrupt status is 0

    uint8_t* lcd_stat = &ppu->memory->io[0x41]; //Pointer to stat register

    //Update PPU mode in stat register
    //Zeros bottom 2 bits and merges with PPU mode to keep it updated
    *lcd_stat = (*lcd_stat & 0xFC) | (ppu->memory->current_ppu_mode & 0x03);

    //If LCY is true, it sets STAT bit 2
    //Otherwise clear it
    if (lyc)
        *lcd_stat |= (1 << 2);
    else
        *lcd_stat &= ~(1 << 2);

    //Trigger interrupt under these conditions..
    if (GET_BIT(*lcd_stat, 3) && (ppu->memory->current_ppu_mode == PPU_MODE_0) && (ppu->memory->previous_ppu_mode != PPU_MODE_0))
        current_stat = 1;
    if (GET_BIT(*lcd_stat, 4) && (ppu->memory->current_ppu_mode == PPU_MODE_1) && (ppu->memory->previous_ppu_mode != PPU_MODE_1))
        current_stat = 1;
    if (GET_BIT(*lcd_stat, 5) && (ppu->memory->current_ppu_mode == PPU_MODE_2) && (ppu->memory->previous_ppu_mode != PPU_MODE_2))
        current_stat = 1;
    if (GET_BIT(*lcd_stat, 6) && GET_BIT(*lcd_stat, 2))
        current_stat = 1;

    //STAT interrupt triggers on rising edge..
    if (ppu->prev_stat == 0 && current_stat == 1) {
        requestInterrupt(INTERRUPT_LCD, ppu->memory);
    }

    //Update stat
    ppu->prev_stat = current_stat;
}

//Writes background/window to frame buffer
//Returns number of ticks processed (returns early if mode ends)
int write_bg_win(PPU* ppu, uint16_t ticks) {
    //I might separate window and bg, but for now
    //this is just a lot of steps to find what we're looking for

    uint8_t ly = ppu->memory->io[0x44]; //LY register
    uint8_t lcdc = ppu->memory->io[0x40]; //LCDC register

    uint32_t start_time = ppu->frame_time; //Start frame time
    uint16_t scanline_time = (start_time) % 456; //Number of ticks on this scanline
    uint16_t mode_3_time = scanline_time - 80; //Number of ticks into mode 3.. This starts on tick 80

    int i = 0;

    //Draw pixels for each tick until 172 have passed
    //If mode_3 time is 172, then we are at the end of mode 3 and need to return...
    //Othewise, do this for as many ticks that have passed
    while (i < ticks && mode_3_time < 172) {
        

        //Update values
        ++i;
        ++scanline_time;
        ++mode_3_time;
    }

    return i; //i is number of ticks processed
}