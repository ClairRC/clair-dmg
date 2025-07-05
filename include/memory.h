#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stddef.h>

#include "mbc_handler.h"

//Memory emulation

//Emulated memory locations
#define DIV_LOCATION io[0x04] //Location of DIV register
#define TIMA_LOCATION io[0x05] //Location of TIMA register
#define TMA_LOCATION io[0x06] //Location of TMA register
#define TAC_LOCATION io[0x07] //Location of TAC register

#define LCDC_LOCATION io[0x40] //Location of LCDC register
#define STAT_LOCATION io[0x41] //LCD Status register
#define SCY_LOCATION io[0x42] //Scroll Y register
#define SCX_LOCATION io[0x43] //Scroll X register
#define LY_LOCATION io[0x44] //Location of LY register
#define LYC_LOCATION io[0x45] //Location of LYC register
#define BGP_LOCATION io[0x47] //Background palette register
#define OBP0_LOCATION io[0x48] //Object palette 0
#define OBP1_LOCATION io[0x49] //Object palette 1
#define WY_LOCATION io[0x4A] //Window X register
#define WX_LOCATION io[0x4B]

/*
* TODO:
* Add 2nd vram bank and wram banks 2-7 with CGB emulation
* Emulate echo ram and unusable ram area for accuracy.
*/

//State flags for memory module
typedef struct {
    //PPU Mode flags
    PPU_Mode current_ppu_mode;
    
    //DMA flags
    uint8_t dma_active;
    uint16_t remaining_dma_cycles;
    uint8_t dma_source;
    uint8_t div_reset; //A write to div resets system clock

    //Bootrom flag
    uint8_t boot_rom_mapped;

    //Current wram bank
    uint8_t current_wram_bank; //Always 1 on DMG. Not related to MBC
} MemoryState;

//Memory struct to hold different memory mappings.
//Full memory map for GB can be found here: https://gbdev.io/pandocs/Memory_Map.html
typedef struct{
    //MBC struct to store information about the Memory Bank Controller
    MBC* mbc_chip;

    //Fixed memory locations
    uint8_t* vram_0; //8kb vram bank. CGB has a second vram bank.
    //TODO: When implementing CGB, change the size of this array! Or just keep it as is, it doesnt really matter.
    uint8_t* wram_x; //4kb wram bank. CGB has 8 total, DMG just has 1
    uint8_t* oam; //Object Attribute Memory
    uint8_t* io; //IO registers
    uint8_t* hram; //hram. Final index is the interrupt enable register.

    //Switchable banks
    uint8_t* rom_x;
    uint8_t* exram_x;

    //Boot ROM to set initial values
    uint8_t* boot_rom;
    uint16_t boot_rom_size;

    //Other memory flags
    MemoryState state;
} Memory;

//Initialize memory
Memory* memory_init(uint8_t, uint8_t, uint8_t);

//Destroy memoy
void memory_destroy(Memory*);

//Read/Write dispatch functions
uint8_t mem_read(Memory*, uint16_t, Accessor);
int mem_write(Memory*, uint16_t, uint8_t, Accessor);

//Read/Write functions
uint8_t rom_read(Memory* mem, uint16_t address, Accessor accessor);
uint8_t vram_read(Memory* mem, uint16_t address, Accessor accessor);
uint8_t exram_read(Memory* mem, uint16_t address, Accessor accessor);
uint8_t wram_read(Memory* mem, uint16_t address, Accessor accessor);
uint8_t oam_read(Memory* mem, uint16_t address, Accessor accessor);
uint8_t io_read(Memory* mem, uint16_t address, Accessor accessor);
uint8_t hram_read(Memory* mem, uint16_t address, Accessor accessor);

int vram_write(Memory* mem, uint16_t address, uint8_t new_val, Accessor accessor);
int exram_write(Memory* mem, uint16_t address, uint8_t new_val, Accessor accessor);
int wram_write(Memory* mem, uint16_t address, uint8_t new_val, Accessor accessor);
int oam_write(Memory* mem, uint16_t address, uint8_t new_val, Accessor accessor);
int io_write(Memory* mem, uint16_t address, uint8_t new_val, Accessor accessor);
int hram_write(Memory* mem, uint16_t address, uint8_t new_val, Accessor accessor);

#endif