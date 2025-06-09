#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stddef.h>

#include "hardware_def.h"

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
    uint8_t rom_0[0x4000]; //16kb rom bank from cartridge
    uint8_t vram_0[0x2000]; //8kb vram bank. CGB has a second vram bank.
    uint8_t wram_0[0x1000]; //4kb wram bank. CGB has more of these
    uint8_t oam[0xA0]; //Object Attribute Memory
    uint8_t io[0x80]; //IO registers
    uint8_t hram[0x80]; //hram. Final index is the interrupt enable register.

    //Switchable banks
    //These change dynamically, so they are handled differently
    //PLEASE keep in mind that the 0th bank is already included for ROM and WRAM
  
    //rom banks
    //Address area is 0x4000 bytes or 16kb
    size_t num_rom_banks;
    size_t current_rom_bank;
    uint8_t* rom_x;

    //exram banks
    //Address area is 0x2000 bytes or 8kb
    size_t num_exram_banks;
    size_t current_exram_bank;
    uint8_t* exram_x;

    //wram banks
    //Address area is 0x1000 bytes or 4kb
    size_t num_wram_banks;
    size_t current_wram_bank;
    uint8_t* wram_x;

    //Special flags/timer for DMA and PPU mode
    PPU_Mode current_ppu_mode;
    int dma_active;
    uint16_t remaining_dma_cycles;
} Memory;

//Initialize memory
Memory* memory_init(size_t, size_t, size_t);

//Destroy memoy
void memory_destroy(Memory*);

//Read/Write dispatch functions
uint8_t mem_read(Memory*, uint16_t, Accessor);
int mem_write(Memory*, uint16_t, uint8_t, Accessor);

#endif