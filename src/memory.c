#include <stdlib.h>
#include <stdio.h>
#include "memory.h"
#include "hardware_def.h"
#include "logging.h"
#include "hardware_registers.h"
#include "mbc_handler.h"

Memory* memory_init(uint8_t mbc_type, uint8_t rom_size_byte, uint8_t ram_size_byte) {
    Memory* mem = (Memory*)malloc(sizeof(Memory));
    
    //if Initialization failes, return NULL
    if (mem == NULL) {
        printError("Failed to initialize memory.");
        return NULL;
    }

    //Get MBC chip
    MBC* mbc_chip = mbc_init(mbc_type, rom_size_byte, ram_size_byte);

    mem->mbc_chip = mbc_chip;

    //Setup ROM and EXRAM arrays
    uint8_t* rom_x = (uint8_t*)malloc(mbc_chip->num_rom_banks * 0x4000);
    uint8_t* exram_x;
    //MBC2 only has 0x200 addresses for RAM access. Weird edge case, but yes
    if (mbc_chip->mbc_type != MBC_2)
        exram_x = (uint8_t*)malloc(mbc_chip->num_exram_banks * 0x2000);
    else 
        exram_x = (uint8_t*)malloc(0x200);

    //Error for failing to initialize this
    if (rom_x == NULL || exram_x == NULL) {
        printError("Failed to intialize memory");
        if (rom_x != NULL) {free(rom_x);}
        if (exram_x != NULL) {free(exram_x);}
        free(mem);
        return NULL;
    }

    mem->rom_x = rom_x;
    mem->exram_x = exram_x;

    //WRAM bank pointer. Always 1 on DMG
    mem->current_wram_bank = 1;

    //Other flags
    mem->current_ppu_mode = 0;
    mem->dma_active = 0;
    mem->remaining_dma_cycles = 640;
    mem->div_reset = 0;
    mem->stat_interrupt_state = 0;

    return mem;
}

void memory_destroy(Memory* mem) {
    //If pointer is null, we're done
    if (mem == NULL)
        return;

    if (mem->rom_x != NULL)
        free(mem->rom_x);

    if (mem->exram_x != NULL)
        free(mem->exram_x);

    free(mem);
}

//Helper functions and enums. These will make the read/write functions less complex
typedef enum {RANGE_ROM, RANGE_VRAM, RANGE_EXRAM, RANGE_WRAM, RANGE_OAM, RANGE_IO} MemoryRange;
int checkNoMemAccess(Memory*, MemoryRange, Accessor); //Checks whether memory can be accessed
uint8_t* getMemPtr(Memory*, uint16_t, Accessor);

//R/W Memory functions
uint8_t mem_read(Memory* mem, uint16_t address, Accessor accessor) {
    uint8_t* mem_ptr = getMemPtr(mem, address, accessor);

    //Null pointer means memory is inaccessible
    if (mem_ptr == NULL) {
        printError("Read attempted from inaccessible address");
        return 0xFF;
    }

    uint8_t result = *mem_ptr; //Value at pointer if its not NULL

    //Hardware registers have some weird quirks
    //Applies a mask to the result so that the correct values get returned for special registers
    if (address >= 0xFF00 && address <= 0xFFFF) {
        HardwareRegister hw_reg = hw_registers[(uint8_t)address]; //Gets hardware register from LSB

        //TODO: Fix this when implementing CGB
        if (hw_reg.cgb_only) {
            printError("Read attempted from inaccessible address");
            return 0xFF;
        }

        //Bits with a 0 are write-only, so this forces those bits to be set as 1, which is correct behavior for this
        result |= ~hw_reg.read_mask;
    }

    //Edge case where MBC2 only accesses lower nibble of exram
    if (mem->mbc_chip->mbc_type == MBC_2 && address >= 0xA000 && address <= 0xBFFF)
        result |= 0xF0; //Sets upper nibble to 1111, preserves lower (accurate behavior)

    //TODO! REMOVE THIS! ONLY FOR TESTING!
    if (address == 0xFF44)
        result = 0x90;

    //Return value
    return result;
}

//Returns 1 if access to memory is blocked or write fails
int mem_write(Memory* mem, uint16_t address, uint8_t new_val, Accessor accessor) {
    uint8_t* mem_ptr = getMemPtr(mem, address, accessor);

    //If null pointer, no write and return 1 (maybe good for debugging later)
    if (mem_ptr == NULL) {
        printError("Write attempted at inaccessible address");
        return 1;
    }

    uint8_t old_val = *mem_ptr; //Gets currently stored value

    //Applies write-mask for weird hardware registers
    if (address >= 0xFF00 && address <= 0xFFFF) {
        HardwareRegister hw_reg = hw_registers[(uint8_t)address];

        //TODO: Implement proper CGB behavior
        if (hw_reg.cgb_only) {
            printError("Write attempted at inaccessible address");
            return 1; //Return 1 and do no write for CGB only registers for now
        }

        //Preserves write bits from new_val and 0s R/O bits, and does the opposite for old_val
        //Bitwise or glues them together
        new_val = (new_val & hw_reg.write_mask) | (old_val & ~hw_reg.write_mask);
    }

    //Edge case where MBC2 only accesses lower nibble of exram
    if (mem->mbc_chip->mbc_type == MBC_2 && address >= 0xA000 && address <= 0xBFFF)
        new_val &= 0x0F; //Sets upper nibble to 0000, preserves lower (accurate behavior)

    //Edge case where 0xFF46 is written to, causing DMA transfer
    if (address == 0x0FF46) {
        mem->dma_active = 1;
        mem->remaining_dma_cycles = 640;
        mem->dma_source = new_val;
    }

    //Edge case where writing to DIV resets it (and the system clock)
    if (address == 0xFF04)
        mem->div_reset = 1;

    *mem_ptr = new_val;

    //Updates the currently addressed ROM/RAM bank
    update_current_bank(mem->mbc_chip, address, new_val);
    
    return 0;
}

/*
* CPU is occasionally locked out of accessing certain parts of memory during DMA transfer
* or certain PPU modes. During DMA transfer on DMG, CPU can only access HRAM.
* (Note this works differently on CGB because of ROM and WRAM using separate buses)
*
* CPU is also locked out of accessing OAM at certain times during certain PPU modes.
* PPU and DMA can always access everything they need.
*/

//TODO: Implement CGB DMA transfer behavior

//Helper function that returns a pointer to the memory that the address is meant to point to
//This makes me only have to do this big if/else chain once instead of twice, and
//the read/write functions will be cleaner with less cluttered logic..
//Returns NULL if memory is currently inaccessible/not properly initialized.
uint8_t* getMemPtr(Memory* mem, uint16_t address, Accessor accessor) {
    //I'm proud of this idea :)

    //Rom bank 0
    if (address <= 0x3FFF) {
        if (checkNoMemAccess(mem, RANGE_ROM, accessor)) {return NULL;}
        
        uint16_t index = address;
        return &(mem->rom_x[index]);
    }

    //Rom bank 1-NN
    else if (address >= 0x4000 && address <= 0x7FFF) {
        //If somehow this gets called with only 0 rom banks (shouldn't happen), return NULL for default value
        if (mem->rom_x == NULL || checkNoMemAccess(mem, RANGE_ROM, accessor)) {return NULL;}
        uint32_t index = (address - 0x4000) + mem->mbc_chip->current_rom_bank * 0x4000;

        return &(mem->rom_x[index]);
    }

    //VRAM bank 0/1
    //TODO: Add logic for VRAM_1 when implementing CGB
    else if(address >= 0x8000 && address <= 0x9FFF) {
        if (checkNoMemAccess(mem, RANGE_VRAM, accessor)) {return NULL;}
        uint16_t index = address - 0x8000;

        return &(mem->vram_0[index]);
    }

    //EXRAM bank 0-NN
    else if(address >= 0xA000 && address <= 0xBFFF) {
        if (mem->exram_x == NULL || checkNoMemAccess(mem, RANGE_EXRAM, accessor)) {return NULL;}
        uint32_t index;

        //Index wraps around, but MBC2 is special because it only has 0x200 addresses
        //This just makes it so if it is MBC2, it'll be a value between 0x0 and 0x200
        if (mem->mbc_chip->mbc_type != MBC_2)
            index = ((address - 0xA000) + (mem->mbc_chip->current_exram_bank * 0x2000)) % (mem->mbc_chip->num_exram_banks * 0x2000);
        else
            index = (address - 0xA000) % 0x200;

        return &(mem->exram_x[index]);
    }

    //WRAM bank 0
    else if(address >= 0xC000 && address <= 0xCFFF) {
        if (checkNoMemAccess(mem, RANGE_WRAM, accessor)) {return NULL;}
        uint16_t index = address - 0xC000;

        return &(mem->wram_x[index]);
    }

    //WRAM bank 1-7 (CGB)
    else if(address >= 0xD000 && address <= 0xDFFF) {
        if(checkNoMemAccess(mem, RANGE_WRAM, accessor)) {return NULL;}
        uint16_t index = (address - 0xD000) + mem->current_wram_bank * 0x1000;

        return &(mem->wram_x[index]);
    }

    //Echo RAM (WRAM bank 0 mirror)
    //This area of memory points to the same physical memory as WRAM bank 0
    //This is because the address bus in the GameBoy for this section only reads the bottom 13 bits
    //As a result, the same memory is accessed in this region as 0xC000-0xDDFF
    else if(address >= 0xE000 && address <= 0xFDFF) {
        if (checkNoMemAccess(mem, RANGE_WRAM, accessor)) {return NULL;}
        uint16_t index = address - 0xE000;

        return &(mem->wram_x[index]);
    }

    //OAM
    else if(address >= 0xFE00 && address <= 0xFE9F) {
        if (checkNoMemAccess(mem, RANGE_OAM, accessor)) {return NULL;}
        uint8_t index = address - 0xFE00;

        return &(mem->oam[index]);
    }

    //Prohibited memory
    else if (address >= 0xFEA0 && address <= 0xFEFF) {
        //This area is prohibited by Nintendo, and has weird behavior.
        //Sometimes it triggers OAM corruption, sometimes it does something else...
        //For now, it will be ignored/return 0xFF
        //TODO: Implement specific behavior
        return NULL;
    }

    //I/O registers
    else if (address >= 0xFF00 && address <= 0xFF7F) {
        if (checkNoMemAccess(mem, RANGE_IO, accessor)) {return NULL;}
        uint8_t index = address - 0xFF00;

        return &(mem->io[index]);
    }

    //HRAM/Interrup enable register (0xFFFF)
    else if (address >= 0xFF80 && address <= 0xFFFF) {
        //I grouped IE register with HRAM, so this will have a special check if it can be accessed
        //(It can Not be accessed during DMA transfer on DMG) (I'll have to add more logic here when CGB gets implemented)
        if (address == 0xFFFF && mem->dma_active) {return NULL;} //IE during DMA transfer
        uint8_t index = address - 0xFF80;

        return &(mem->hram[index]);
    }

    //Other (Should HOPEFULLY never happen)
    else
        return NULL; 
}


//Helper function to determine accessibility of different memory ranges during DMA or PPU modes
//Returns 1 if no access, 0 if yes access
int checkNoMemAccess(Memory* mem, MemoryRange range, Accessor accessor) {
    //As far as I know, there is no restriction except for CPU access
    //This may be different on CGB
    //TODO: confirm that ^
    if (accessor != CPU_ACCESS)
        return 0;

    switch (range) {
        case RANGE_ROM:
            //No access during DMA
            return mem->dma_active;
            break;

        case RANGE_VRAM:
            //No access during DMA or PPU mode 3
            return mem->dma_active 
                || mem->current_ppu_mode == PPU_MODE_3;
            break;

        case RANGE_EXRAM:
            //No access during DMA or when EXRAM is disabled by the MBC
            return mem->dma_active || mem->mbc_chip->exram_enabled == 0;
            break;

        case RANGE_WRAM:
            //No access during DMA
            return mem->dma_active;
            break;

        case RANGE_OAM:
            //No access during DMA or PPU mode 2 or 3
            return mem->dma_active
                || mem->current_ppu_mode == PPU_MODE_2
                || mem->current_ppu_mode == PPU_MODE_3;
                break;

        case RANGE_IO:
            //No access during DMA
            return mem->dma_active;
            break;

        default:
            //Should never get here, but can never be too sure.
            return 0;
    }
}