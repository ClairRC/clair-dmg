#ifndef PPU_H
#define PPU_H 

#include <stdint.h>
#include "memory.h"
#include "hardware_def.h"

//This is the Pixel Processing Unit, which is the GameBoy's screen basically.

//Struct for Object attributes, which will help drawing a lot
typedef struct {
    uint8_t y_pos;
    uint8_t x_pos;
    uint8_t tile_index;
    uint8_t flags;
} OAM_Entry;

typedef struct {
    Memory* memory; //Memory
    uint32_t frame_time; //Elapsed frame time
    uint8_t vblank_active; //Whether PPU is in vblank mode

    //Scanline OAM entires
    OAM_Entry oam_indices[10];
    int current_oam_index; //Tracks the current index of the array

    //Frame buffer and current palette data for drawing...
    //TODO: Possibly move this and all that.
    uint8_t framebuffer[160 * 144]; //Gameboy is 160x144 pixels
    uint8_t palette[4]; //Gameboy has 4 colors...
} PPU;

PPU* ppu_init(Memory*);
void ppu_destroy(PPU*);
void update_ppu(PPU*, uint16_t);
void ppu_oam_scan(PPU*, uint16_t);
void ppu_write_lcd(PPU*, uint16_t);
void update_ppu_mode(PPU*);
void draw(PPU*);

#endif 