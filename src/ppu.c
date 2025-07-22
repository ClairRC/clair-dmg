#include "ppu.h"
#include "hardware_def.h"
#include "logging.h"
#include "interrupt_handler.h"
#include <stdlib.h>

//Inline functions
#define GET_BIT(num, bit) ((num) >> (bit)) & 0x1 //Gets value of specific bit (starting at 0)

PPU* ppu_init(MemoryBus* bus, GlobalPPUState* global_state, SDL_Display_Data* sdl_data) {
    if (bus == NULL || global_state == NULL || sdl_data == NULL) {
        printError("Unable to initialize PPU.");
        return NULL;
    }

    PPU* ppu = (PPU*)malloc(sizeof(PPU));

    if (ppu == NULL) {
        printError("Unable to initialize PPU.");
        return NULL;
    }

    ppu->bus = bus;
    ppu->global_state = global_state;
    ppu->sdl_data = sdl_data; //SDL Video data for rendering

    ppu->local_state.prev_stat = 0; //Previous STAT flag for stat 
    ppu->local_state.window_ly = 0;
    ppu->local_state.window_ly_increment = 0;
    ppu->local_state.current_obj_index = 0;
    ppu->local_state.pixel_obj_index = 0;

    ppu->framebuffer = (uint32_t*)calloc((160 * 144), sizeof(uint32_t)); //Gameboy is 160x144

    //Palette grey-scale colors...
    ppu->palette = (PaletteData*)calloc(1, sizeof(PaletteData));
    
    if (ppu->framebuffer == NULL || ppu->palette == NULL) {
        printError("Error initializing PPU");
        ppu_destroy(ppu);
        return NULL;
    }

    //For now, emulator only has 1 palette
    uint32_t colors[4] = { PALETTE_WHITE, PALETTE_LIGHT_GRAY, PALETTE_DARK_GRAY, PALETTE_BLACK };

    memcpy(ppu->palette->BG_Palette, colors, 4 * sizeof(uint32_t));
    memcpy(ppu->palette->OBJ0_Palette, colors, 4 * sizeof(uint32_t));
    memcpy(ppu->palette->OBJ1_Palette, colors, 4 * sizeof(uint32_t));

    return ppu;
}

void ppu_destroy(PPU* ppu) {
    if (ppu == NULL)
        return;

    if (ppu->framebuffer != NULL) { free(ppu->framebuffer); }
    if (ppu->palette != NULL) { free(ppu->palette); }
}

//Updates PPU based on current frame time
void update_ppu(PPU* ppu) {
    //Update PPU state
    update_ppu_state(ppu);

    //Update stat register and request interrupt if necessary
    update_stat(ppu);

    //Depending on state, do different things
    switch (ppu->global_state->current_mode) {
    case PPU_MODE_2:
        ppu_oam_scan(ppu);
        break;

    case PPU_MODE_3:
        ppu_write_lcd(ppu);
        break;
    }
}

//Does OAM scan
void ppu_oam_scan(PPU* ppu) {
    //OAM scan takes 80 ticks, but only reads 40 objects
    //This means it takes 2 ticks per object, but for the sake of
    //this emulator, I am just going to do 1 object every other tick

    uint8_t mode_2_time = ppu->global_state->frame_time % SCANLINE_END;

    //Additionally, each scanline can only have 10 objects. On DMG, this is the first 10 suitable objects
    //in OAM, so if we already have 10, then we also don't want to do anything
    if (mode_2_time % 2 != 0 || ppu->local_state.current_obj_index >= 10)
        return;

    uint8_t obj_height = (ppu->bus->memory->LCDC_LOCATION & OBJ_SIZE) ? 16 : 8; //if bit is set, objects are 8x16, if its clear they're 8x8
    uint8_t obj_offset = (mode_2_time / 2) * 4; //Each attribute is 4 bytes, only once per 2 frames.
    uint16_t obj_base_address = 0xFE00 + obj_offset;

    //Y value is at a 16 pixel offset.. I'm casting this to a 16-bit signed int to avoid underflow
    int16_t obj_y = (int16_t)mem_read(ppu->bus, obj_base_address, PPU_ACCESS); //Byte 0 is y-value
    obj_y = obj_y - 16; //Corresponds to the top-most scanline this object touches

    uint8_t ly = ppu->bus->memory->LY_LOCATION;
    //If object is on scanline, add it to object list.
    if (obj_y <= ly && ly < obj_y + obj_height) {
        store_object(ppu, obj_base_address);
    }
}


//Writes the color value to the frame buffer
void ppu_write_lcd(PPU* ppu) {
    uint8_t mode_3_time = (ppu->global_state->frame_time % 456) - 80;
    uint8_t color_id = get_pixel_color_id(ppu, mode_3_time);

    //Get framebuffer index and ouput to frame buffer
    uint32_t framebuffer_i = 160 * ppu->bus->memory->LY_LOCATION + mode_3_time;
    ppu->framebuffer[framebuffer_i] = ppu->palette->BG_Palette[color_id];
}

void update_stat(PPU* ppu) {
    //LYC is if LYC register and LY are equal
    uint8_t lyc = ppu->bus->memory->LY_LOCATION == ppu->bus->memory->LYC_LOCATION;
    uint8_t current_stat = 0; //Current interrupt status is 0 by default

    uint8_t* lcd_stat = &ppu->bus->memory->STAT_LOCATION; //Pointer to stat register

    //Current PPU mode
    PPU_Mode current_mode = ppu->global_state->current_mode;

    //Update PPU mode in stat register
    //Zeros bottom 2 bits and merges with PPU mode to keep it updated
    *lcd_stat = (*lcd_stat & 0xFC) | (current_mode & 0x03);

    //If LCY is true, it sets STAT bit 2
    //Otherwise clear it
    if (lyc)
        *lcd_stat |= (1 << 2);
    else
        *lcd_stat &= ~(1 << 2);

    //Trigger interrupt under these conditions..
    if (GET_BIT(*lcd_stat, 3) && (ppu->global_state->frame_time % SCANLINE_END) == MODE_3_END)
        current_stat = 1;
    if (GET_BIT(*lcd_stat, 4) && ppu->global_state->frame_time == VBLANK_BEGIN)
        current_stat = 1;
    if (GET_BIT(*lcd_stat, 5) && (ppu->global_state->frame_time % SCANLINE_END) == 0)
        current_stat = 1;
    if (GET_BIT(*lcd_stat, 6) && GET_BIT(*lcd_stat, 2))
        current_stat = 1;

    //STAT interrupt triggers on rising edge..
    if (ppu->local_state.prev_stat == 0 && current_stat == 1) {
        requestInterrupt(INTERRUPT_LCD, ppu->bus->memory);
    }

    //Update stat
    ppu->local_state.prev_stat = current_stat;
}

//Switch from mode 0 to mode 1 (hblank to vblank)
void switch_mode_0_1(PPU* ppu) {
    //If we just started vblank, request interrupt now
    requestInterrupt(INTERRUPT_VBLANK, ppu->bus->memory);

    ppu->local_state.window_ly = 0; //Reset internal window counter
    ppu->global_state->current_mode = PPU_MODE_1; //Update PPU mode
}

//Switch from mode 1 to mode 2 (vblank to oam scan)
void switch_mode_1_2(PPU* ppu) {
    //At end of VBLANK
    ppu->global_state->current_mode = PPU_MODE_2; //Update current PPU mode
    ppu->global_state->frame_time = 0; //Reset frame time back to 0

    //Draws buffer through SDL and waits to maintain framerate
    draw_buffer(ppu->sdl_data, ppu->framebuffer, ppu->global_state->frame_rate);
}

//Switch from mode 2 to mode 3 (oam scan to draw scanline)
void switch_mode_2_3(PPU* ppu) {
    ppu->local_state.window_ly_increment = 1; //Allow window LY to get incremented again for this scanline
    ppu->global_state->current_mode = PPU_MODE_3; //Update PPU mode to mode 3
}

//Switch from mode 3 to mode 0 (draw scanline to hblank)
void switch_mode_3_0(PPU* ppu) {
    ppu->local_state.current_obj_index = 0; //Reset scanline sprite index
    ppu->global_state->current_mode = PPU_MODE_0;
}

//Switch from mode 0 to mode 2 (hblank to oam scan)
void switch_mode_0_2(PPU* ppu) {
    ppu->global_state->current_mode = PPU_MODE_2;
}