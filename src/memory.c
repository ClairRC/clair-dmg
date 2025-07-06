#include <stdlib.h>
#include <stdio.h>
#include "memory.h"
#include "hardware_def.h"
#include "logging.h"
#include "hardware_registers.h"
#include "mbc_handler.h"

Memory* memory_init(uint8_t mbc_type, uint8_t rom_size_byte, uint8_t ram_size_byte, SDL_Data* sdl_data) {
    Memory* mem = (Memory*)malloc(sizeof(Memory));
    
    //if Initialization failes, return NULL
    if (mem == NULL) {
        printError("Failed to initialize memory.");
        return NULL;
    }

    //SDL struct
    mem->sdl_data = sdl_data;

    //Get MBC chip
    MBC* mbc_chip = mbc_init(mbc_type, rom_size_byte, ram_size_byte);

    if (mbc_chip == NULL)
        return NULL;

    mem->mbc_chip = mbc_chip;

    //Setup Memory Arrays
    
    //Fixed memory locations
    //TODO: Add error for these being NULL
    mem->vram_0 = (uint8_t*)calloc(0x2000, 1); //8kb vram bank. CGB has a second vram bank.
    mem->wram_x = (uint8_t*)calloc(0x2000, 1); //TODO: DMG has 2 fixed banks, CBG has 1 fixed and 7 switchable
    mem->oam = (uint8_t*)calloc(0xA0, 1); //Object Attribute Memory
    mem->io = (uint8_t*)calloc(0x80, 1); //IO registers
    mem->hram = (uint8_t*)calloc(0x80, 1); //hram. Final index is the interrupt enable register.

    //Switchable bank locations
    mem->rom_x = (uint8_t*)calloc(mbc_chip->num_rom_banks * 0x4000, 1); //ROM always has at least 1 of these, typically 2
    mem->exram_x = NULL; //There may be no exram
    //MBC2 has 0x200 addresses for RAM access. Weird edge case, but yes
    if (mbc_chip->mbc_type == MBC_2)
        mem->exram_x = (uint8_t*)calloc(0x200, 1);
    else if (mbc_chip->num_exram_banks != 0)
        mem->exram_x = (uint8_t*)calloc(mbc_chip->num_exram_banks * 0x2000, 1); //If MBC is not 2, and there are ram banks, initialize

    //Error for failing to initialize any memory
    //RAM is allowed to be NULL
    if (mem->rom_x == NULL) {
        printError("Failed to intialize memory");
        free(mem->rom_x);
        free(mem);
        return NULL;
    }

    //By default boot_rom will stay null
    //TODO: Add better intialization for bootrom
    mem->boot_rom = NULL;
    mem->boot_rom_size = 0;

    //Other flags
    mem->state.current_ppu_mode = PPU_MODE_2;

    mem->state.dma_active = 0;
    mem->state.remaining_dma_cycles = 640; //DMA takes 640 ticks
    mem->state.div_reset = 0; //A write to div resets system clock

    //Bootrom flag
    mem->state.boot_rom_mapped = 0;

    //Current wram bank
    mem->state.current_wram_bank = 1; //Always 1 on DMG. Not related to MBC
   

    //TODO: Refactor input stuff
    //Start with no buttons pressed
    mem->button_state = 0x0F;
    mem->dpad_state = 0x0F;
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

//Read dispatch function
uint8_t mem_read(Memory* mem, uint16_t address, Accessor accessor) {
    uint8_t result = 0xFF; //Default value

    //TODO: Remove this
    //if (address == 0xFF00)
        //printf("");
        //return result;

    //Call correct function based on memory area
    if (address >= 0x0000 && address <= 0x7FFF)
        result = rom_read(mem, address, accessor); //ROM area
    else if (address >= 0x8000 && address <= 0x9FFF)
        result = vram_read(mem, address, accessor); //VRAM area
    else if (address >= 0xA000 && address <= 0xBFFF)
        result = exram_read(mem, address, accessor); //EXRAM area
    else if (address >= 0xC000 && address <= 0xDFFF)
        result = wram_read(mem, address, accessor); //WRAM area
    else if (address >= 0xE000 && address <= 0xFDFF)
        result = wram_read(mem, address - 0x2000, accessor); //Echo RAM, which is WRAM, so offset the address to account for that
    else if (address >= 0xFE00 && address <= 0xFE9F)
        result = oam_read(mem, address, accessor); //OAM area
    else if (address >= 0xFEA0 && address <= 0xFEFF)
        printError("Read attemtped at prohibited memory"); //Unusable area, prohibited by Nintendo, so keep default value
    else if (address >= 0xFF00 && address <= 0xFF7F)
        result = io_read(mem, address, accessor); //I/O registers
    else if (address >= 0xFF80)
        result = hram_read(mem, address, accessor); //HRAM (and also IE)

    return result;
}

//Write dispatch function. Retursn 1 if write failed for any reason
int mem_write(Memory* mem, uint16_t address, uint8_t new_val, Accessor accessor) {
    int success = 1; //Returns 0 if write was successful

    //Call correct function based on memory area
    if (address >= 0x0000 && address <= 0x7FFF)
        printError("Write attempted at ROM area"); //ROM area (not writable)
    else if (address >= 0x8000 && address <= 0x9FFF)
        success = vram_write(mem, address, new_val, accessor); //VRAM area
    else if (address >= 0xA000 && address <= 0xBFFF)
        success = exram_write(mem, address, new_val, accessor); //EXRAM area
    else if (address >= 0xC000 && address <= 0xDFFF)
        success = wram_write(mem, address, new_val, accessor); //WRAM area
    else if (address >= 0xE000 && address <= 0xFDFF)
        success = wram_write(mem, address - 0x2000, new_val, accessor); //Echo RAM, which is WRAM, so offset the address to account for that
    else if (address >= 0xFE00 && address <= 0xFE9F)
        success = oam_write(mem, address, new_val, accessor); //OAM area
    else if (address >= 0xFEA0 && address <= 0xFEFF)
        printError("Write attempted at prohibited memory"); //Unusable area, prohibited by Nintendo, so keep default value
    else if (address >= 0xFF00 && address <= 0xFF7F)
        success = io_write(mem, address, new_val, accessor); //I/O registers
    else if (address >= 0xFF80)
        success = hram_write(mem, address, new_val, accessor); //HRAM (and also IE)

    //Updates MBC flags
    update_current_bank(mem->mbc_chip, address, new_val);

    return success;
}

/* * * * * *
*          *
* ROM Read *
*          *
* * * * * */
uint8_t rom_read(Memory* mem, uint16_t address, Accessor accessor) {
    //Check if area is readable
    if (mem->state.dma_active && accessor == CPU_ACCESS) {
        printError("Read to ROM area during DMA transfer");
        return 0xFF;
    }

    //Get index

    uint32_t index;
    //If BOOT rom is enabled and reading from it return that
    if (address < mem->boot_rom_size) {
        if (mem->state.boot_rom_mapped) {
            return mem->boot_rom[address];
        }
    }

    if (address <= 0x3FFF) {
        index = address; //Always bank 0 on non mbc1 cartridges

        //On MBC1 in mode 1, this bank is a little weirder
        if (mem->mbc_chip->mbc_type == MBC_1) {
            //In mode 1, ROM bank is the value of BANK2, which is the upper 3 bits in this case
            if (mem->mbc_chip->mbc_mode == 1) {
                index = address + ((mem->mbc_chip->current_rom_bank & ~0x9F) * 0x4000); //Switchable
            }
        }
    }
    else
        index = (address - 0x4000) + (mem->mbc_chip->current_rom_bank * 0x4000); //Switchable

    return mem->rom_x[index];
}

/* * * * * * * * *
*                 *
* VRAM Read/Write *
*                 *
* * * * * * * * * */
uint8_t vram_read(Memory* mem, uint16_t address, Accessor accessor) {
    //Check if VRAM is readable
    if (mem->state.current_ppu_mode == PPU_MODE_3 && accessor == CPU_ACCESS) {
        printError("Read to VRAM area during mode 3");
        return 0xFF;
    }

    if (mem->state.dma_active && accessor == CPU_ACCESS) {
        printError("Read to VRAM area during DMA transfer");
        return 0xFF;
    }

    //Get index (always bank 0 on DMG)
    uint32_t index = address - 0x8000;

    return mem->vram_0[index];
}

int vram_write(Memory* mem, uint16_t address, uint8_t new_val, Accessor accessor) {
    //Check if VRAM is writable
    if (mem->state.current_ppu_mode == PPU_MODE_3 && accessor == CPU_ACCESS) {
        printError("Write to VRAM area during mode 3");
        return 1;
    }

    if (mem->state.dma_active && accessor == CPU_ACCESS) {
        printError("Write to VRAM area during DMA transfer");
        return 1;
    }

    //Get index (always bank 0 on DMG)
    uint32_t index = address - 0x8000;

    mem->vram_0[index] = new_val;

    return 0;
}

/* * * * * * * * * *
*                  *
* EXRAM Read/Write *
*                  *
* * * * * * * * * */
uint8_t exram_read(Memory* mem, uint16_t address, Accessor accessor) {
    //Check if external RAM is readable
    if (mem->exram_x == NULL) {
        printError("Read to nonexistant EXRAM");
        return 0xFF;
    }

    if (mem->mbc_chip->exram_enabled == 0) {
        printError("Read to EXRAM when disabled");
        return 0xFF;
    }

    if (mem->state.dma_active && accessor == CPU_ACCESS) {
        printError("Read to EXRAM during DMA transfer");
        return 0xFF;
    }

    //Get index
    uint32_t index;
    if (mem->mbc_chip->mbc_type != MBC_2)
        index = (((address - 0xA000) + (mem->mbc_chip->current_exram_bank * 0x2000))) % (mem->mbc_chip->num_exram_banks * 0x2000);
    else
        index = (address - 0xA000) % 0x200; //MBC2 only has 0x200 bytes for some reason

    uint8_t val = mem->exram_x[index];
    if (mem->mbc_chip->mbc_type == MBC_2)
        val |= 0xF0; //MBC 2 only returns the lower nibble

    return val;
}

int exram_write(Memory* mem, uint16_t address, uint8_t new_val, Accessor accessor) {
    //Check if external RAM is readable
    if (mem->exram_x == NULL) {
        printError("Write to nonexistant EXRAM");
        return 1;
    }

    if (mem->mbc_chip->exram_enabled == 0) {
        printError("Write to EXRAM when disabled");
        return 1;
    }

    if (mem->state.dma_active && accessor == CPU_ACCESS) {
        printError("Write to EXRAM during DMA transfer");
        return 1;
    }

    //Get index
    uint32_t index;
    if (mem->mbc_chip->mbc_type != MBC_2)
        index = (((address - 0xA000) + (mem->mbc_chip->current_exram_bank * 0x2000))) % (mem->mbc_chip->num_exram_banks * 0x2000);
    else
        index = (address - 0xA000) % 0x200; //MBC2 only has 0x200 bytes for some reason

    //Edge cases
    if (mem->mbc_chip->mbc_type == MBC_2)
        new_val &= 0x0F; //Sets upper nibble to 0000, preserves lower (accurate behavior)

    mem->exram_x[index] = new_val;

    return 0;
}

/* * * * * * * * *
*                 *
* WRAM Read/Write *
*                 *
* * * * * * * * * */
uint8_t wram_read(Memory* mem, uint16_t address, Accessor accessor) {
    //Check if WRAM is readable
    if (mem->state.dma_active && accessor == CPU_ACCESS) {
        printError("Read to WRAM during DMA transfer");
        return 0xFF;
    }

    //Get index (No switchable banks on DMG)
    uint32_t index = address - 0xC000;

    return mem->wram_x[index];
}

int wram_write(Memory* mem, uint16_t address, uint8_t new_val, Accessor accessor) {
    //Check if WRAM is readable
    if (mem->state.dma_active && accessor == CPU_ACCESS) {
        printError("Write to WRAM during DMA transfer");
        return 1;
    }

    //Get index (No switchable banks on DMG)
    uint32_t index = address - 0xC000;

    mem->wram_x[index] = new_val;
    
    return 0;
}

/* * * * * * * * *
*                *
* OAM Read/Write *
*                *
* * * * * * * * */
uint8_t oam_read(Memory* mem, uint16_t address, Accessor accessor) {
    //Check if OAM is readable
    if (mem->state.current_ppu_mode == PPU_MODE_2 && accessor == CPU_ACCESS) {
        printError("Read to OAM during mode 2");
        return 0xFF;
    }
    
    if (mem->state.current_ppu_mode == PPU_MODE_3 && accessor == CPU_ACCESS) {
        printError("Read to OAM during mode 3");
        return 0xFF;
    }
    
    if (mem->state.dma_active && accessor == CPU_ACCESS) {
        printError("Read to OAM during DMA transfer");
        return 0xFF;
    }

    //Get index
    uint32_t index = address - 0xFE00;

    return mem->oam[index];
}

int oam_write(Memory* mem, uint16_t address, uint8_t new_val, Accessor accessor) {
    //Check if OAM is readable
    if (mem->state.current_ppu_mode == PPU_MODE_2 && accessor == CPU_ACCESS) {
        printError("Write to OAM during mode 2");
        return 1;
    }

    if (mem->state.current_ppu_mode == PPU_MODE_3 && accessor == CPU_ACCESS) {
        printError("Write to OAM during mode 3");
        return 1;
    }

    if (mem->state.dma_active && accessor == CPU_ACCESS) {
        printError("Write to OAM during DMA transfer");
        return 1;
    }

    //Get index
    uint32_t index = address - 0xFE00;

    mem->oam[index] = new_val;

    return 0;
}

/* * * * * * * *
*               *
* IO Read/Write *
*               *
* * * * * * * * */
uint8_t io_read(Memory* mem, uint16_t address, Accessor accessor) {
    //Get index
    uint32_t index = address - 0xFF00;

    //Apply bitmask to hardware registers
    uint8_t val = mem->io[index];

    HardwareRegister hw_reg = hw_registers[(uint8_t)address]; //Gets hardware register from LSB
    if (hw_reg.cgb_only) {
        printError("Read attempted from CGB only hardware register");
        return 0xFF;
    }

    //Bits with a 0 are write-only, so this forces those bits to be set as 1, which is correct behavior for this
    val |= ~hw_reg.read_mask;

    //TODO: Fix input stuff..
    //Currently, the goal is to store the button state at the end of the frame,
    //then when this is read return the correct state based on the selector bit.
    //This should work?
    if (address == 0xFF00) {
        uint8_t joypad_state = 0x0;

        //Read buttons
        if (!(mem->io[0x0] & 1 << 4)) {
            joypad_state |= mem->dpad_state;
        }

        if (!(mem->io[0x0] & 1 << 5)) {
            joypad_state |= mem->button_state;
        }

        //Keep top nibble, replace bottom nibble with button inputs
        val = (val & 0xF0) | joypad_state;
    }

    return val;
}

int io_write(Memory* mem, uint16_t address, uint8_t new_val, Accessor accessor) {
    //Get index
    uint32_t index = address - 0xFF00;

    //Apply bitmask to hardware registers
    uint8_t old_val = mem->io[index];
    HardwareRegister hw_reg = hw_registers[(uint8_t)address]; //Gets hardware register from LSB
    if (hw_reg.cgb_only) {
        printError("Write attempted to CGB only hardware register");
        return 1;
    }

    //Bits with a 0 are read-only, so this forces those bits to not be updated, which is correct behavior for this
    new_val = (new_val & hw_reg.write_mask) | (old_val & ~hw_reg.write_mask);

    /*
    * Edge cases
    */

    //Writes here disable boot rom
    if (address == 0xFF50 && mem->state.boot_rom_mapped == 1) {
        if (mem->boot_rom != NULL) { free(mem->boot_rom); }
        mem->state.boot_rom_mapped = 0;
    }

    //This register begins DMA transfer
    if (address == 0xFF46) {
        mem->state.dma_active = 1;
        mem->state.dma_source = new_val;
    }

    //Writes here reset system clock
    if (address == 0xFF04)
        mem->state.div_reset = 1;

    //Set new value
    mem->io[index] = new_val;

    return 0;
}

/* * * * * * * * *
*                 *
* HRAM Read/Write *
*                 *
* * * * * * * * * */
uint8_t hram_read(Memory* mem, uint16_t address, Accessor accessor) {
    //Get index
    uint16_t index = address - 0xFF80;

    uint8_t val = mem->hram[index];

    return val;
}

int hram_write(Memory* mem, uint16_t address, uint8_t new_val,  Accessor accessor) {
    //Get index
    uint16_t index = address - 0xFF80;

    uint8_t old_val = mem->hram[index];

    mem->hram[index] = new_val;

    return 0;
}

void save_state(Memory* mem);
void load_state(Memory* mem);

//Polls SDL events
//TODO: This should NOT be in this file but I haven't thought of how to organize it logically so I'm keeping
//it here for now JUST for testing and what not
void poll_events(Memory* mem) {
    SDL_Data* sdl = mem->sdl_data;
    uint8_t button_state = mem->button_state;
    uint8_t dpad_state = mem->dpad_state;

    SDL_Event e;

    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT)
            sdl->running = 0;

        if (e.type == SDL_KEYDOWN) {
            //A button
            if (e.key.keysym.sym == SDLK_z)
                button_state &= ~(1 << 0);
            //B button
            if (e.key.keysym.sym == SDLK_x)
                button_state &= ~(1 << 1);
            //Select
            if (e.key.keysym.sym == SDLK_RSHIFT)
                button_state &= ~(1 << 2);
            //Start
            if (e.key.keysym.sym == SDLK_RETURN)
                button_state &= ~(1 << 3);


            //Right
            if (e.key.keysym.sym == SDLK_RIGHT)
                dpad_state &= ~(1 << 0);
            //Left
            if (e.key.keysym.sym == SDLK_LEFT)
                dpad_state &= ~(1 << 1);
            //Up
            if (e.key.keysym.sym == SDLK_UP)
                dpad_state &= ~(1 << 2);
            //Down
            if (e.key.keysym.sym == SDLK_DOWN)
                dpad_state &= ~(1 << 3);

            //Toggle unlimited FPS
            //TODO: fix this lowkey
            if (e.key.keysym.sym == SDLK_SPACE) {
                if (mem->sdl_data->frame_rate == -1)
                    mem->sdl_data->frame_rate = 59.73;
                else
                    mem->sdl_data->frame_rate = -1;
            }
        }

        if (e.type == SDL_KEYUP) {
            //A button
            if (e.key.keysym.sym == SDLK_z)
                button_state |= (1 << 0);
            //B button
            if (e.key.keysym.sym == SDLK_x)
                button_state |= (1 << 1);
            //Select
            if (e.key.keysym.sym == SDLK_RSHIFT)
                button_state |= (1 << 2);
            //Start
            if (e.key.keysym.sym == SDLK_RETURN)
                button_state |= (1 << 3);


            //Right
            if (e.key.keysym.sym == SDLK_RIGHT)
                dpad_state |= (1 << 0);
            //Left
            if (e.key.keysym.sym == SDLK_LEFT)
                dpad_state |= (1 << 1);
            //Up
            if (e.key.keysym.sym == SDLK_UP)
                dpad_state |= (1 << 2);
            //Down
            if (e.key.keysym.sym == SDLK_DOWN)
                dpad_state |= (1 << 3);
        }
    }

    mem->button_state = button_state & 0x0F;
    mem->dpad_state = dpad_state & 0x0F;
}