#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stddef.h>

#include "mbc_handler.h"
#include "sdl_data.h"

//Memory emulation

//Emulated memory locations
#define DIV_LOCATION io[0x04] //Location of DIV register
#define TIMA_LOCATION io[0x05] //Location of TIMA register
#define TMA_LOCATION io[0x06] //Location of TMA register
#define TAC_LOCATION io[0x07] //Location of TAC register

//PPU Locations
#define LCDC_LOCATION io[0x40] //Location of LCDC register
#define STAT_LOCATION io[0x41] //LCD Status register
#define SCY_LOCATION io[0x42] //Scroll Y register
#define SCX_LOCATION io[0x43] //Scroll X register
#define LY_LOCATION io[0x44] //Location of LY register
#define LYC_LOCATION io[0x45] //Location of LYC register
#define BGP_LOCATION io[0x47] //Background palette register
#define OBP0_LOCATION io[0x48] //Object palette 0
#define OBP1_LOCATION io[0x49] //Object palette 1
#define WY_LOCATION io[0x4A] //Window Y register
#define WX_LOCATION io[0x4B] //Window X register

//APU Locations
#define NR10_LOCATION io[0x10]
#define NR11_LOCATION io[0x11]
#define NR12_LOCATION io[0x12]
#define NR13_LOCATION io[0x13]
#define NR14_LOCATION io[0x14]
#define NR21_LOCATION io[0x16]
#define NR22_LOCATION io[0x17]
#define NR23_LOCATION io[0x18]
#define NR24_LOCATION io[0x19]
#define NR30_LOCATION io[0x1A]
#define NR31_LOCATION io[0x1B]
#define NR32_LOCATION io[0x1C]
#define NR33_LOCATION io[0x1D]
#define NR34_LOCATION io[0x1E]
#define NR41_LOCATION io[0x20]
#define NR42_LOCATION io[0x21]
#define NR43_LOCATION io[0x22]
#define NR44_LOCATION io[0x23]
#define NR50_LOCATION io[0x24]
#define NR51_LOCATION io[0x25]
#define NR52_LOCATION io[0x26]

/*
* TODO:
* Add 2nd vram bank and wram banks 2-7 with CGB emulation
* Emulate echo ram and unusable ram area for accuracy.
*/

//Enum for memory range. Includeing ROM, VRAM, I/O, etc
typedef enum {
    RANGE_ROM,
    RANGE_VRAM,
    RANGE_EXRAM,
    RANGE_WRAM,
    RANGE_OAM,
    RANGE_PROHIBITED,
    RANGE_IO,
    RANGE_HRAM
}MemoryRange;

//Struct for information regarding accessed memory
typedef struct {
    MemoryRange range; //Range being accessed
    uint8_t* mem_ptr; //Pointer to memory value
} MemoryValue;

//State flags for memory module
typedef struct {
    //Bootrom flag
    uint8_t boot_rom_mapped; //Bootrom flag
    uint8_t current_wram_bank; //Current WRAM bank. Always 1 on DMG. Not related to MBC
    uint8_t button_state; //Current state of non-dpad buttons
    uint8_t dpad_state; //Current state of dpad buttons
} LocalMemoryState;

//Memory struct to hold different memory mappings.
//Full memory map for GB can be found here: https://gbdev.io/pandocs/Memory_Map.html
typedef struct {
    //MBC struct to store information about the Memory Bank Controller
    MBC* mbc_chip;

    //Fixed memory locations
    uint8_t* vram_0; //8kb vram bank. CGB has a second vram bank.
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
    LocalMemoryState local_state;
} Memory;

//Initialization
Memory* memory_init(FILE* rom_file, FILE* save_data, FILE* boot_rom_file);
MBC_Data* get_mbc_data(FILE* rom_file);
uint8_t load_rom_data(Memory* mem, FILE* rom_file);
uint8_t load_save_data(Memory* mem, FILE* save_data);

//Destroy memoy
void memory_destroy(Memory* mem);

//Returns accessed memory pointer and the range its in
MemoryValue get_memory_value(Memory* mem, uint16_t address);
uint8_t* get_rom_ptr(Memory* mem, uint16_t address);
uint8_t* get_vram_ptr(Memory* mem, uint16_t address);
uint8_t* get_exram_ptr(Memory* mem, uint16_t address);

//Disable bootrom
void disable_bootrom(Memory* mem);

#endif