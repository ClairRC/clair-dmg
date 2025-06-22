#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stddef.h>

#include "mbc_handler.h"

//Memory emulation

/*
* TODO:
* Add 2nd vram bank and wram banks 2-7 with CGB emulation
* Emulate echo ram and unusable ram area for accuracy.
*/

//Memory struct to hold different memory mappings.
//Full memory map for GB can be found here: https://gbdev.io/pandocs/Memory_Map.html
typedef struct{
    //Fixed memory locations
    uint8_t vram_0[0x2000]; //8kb vram bank. CGB has a second vram bank.
    //TODO: When implementing CGB, change the size of this array! Or just keep it as is, it doesnt really matter.
    uint8_t wram_x[0x8000]; //4kb wram bank. CGB has 8 total, DMG just has 1
    uint8_t oam[0xA0]; //Object Attribute Memory
    uint8_t io[0x80]; //IO registers
    uint8_t hram[0x80]; //hram. Final index is the interrupt enable register.

    //MBC struct to store information about the Memory Bank Controller
    MBC* mbc_chip;

    //Switchable banks
    //These change dynamically, so they are handled differently
    uint8_t* rom_x;
    uint8_t* exram_x;

    //These are always included on CGB, but this variable is still needed to know which to access
    size_t current_wram_bank;

    //Special flags/timer for DMA and PPU mode
    PPU_Mode current_ppu_mode;
    PPU_Mode previous_ppu_mode; //Check for mode transition
    int dma_active;
    uint16_t remaining_dma_cycles;
    uint8_t dma_source;
    uint8_t div_reset; //A write to div resets system clock
} Memory;

//Initialize memory
Memory* memory_init(uint8_t, uint8_t, uint8_t);

//Destroy memoy
void memory_destroy(Memory*);

//Read/Write dispatch functions
uint8_t mem_read(Memory*, uint16_t, Accessor);
int mem_write(Memory*, uint16_t, uint8_t, Accessor);

#endif