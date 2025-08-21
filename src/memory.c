#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "memory.h"
#include "hardware_def.h"
#include "logging.h"
#include "hardware_registers.h"
#include "mbc_handler.h"

//Checks which system emulator is compiled on
//This is only to create a new directory for save data if it doesnt exist
#if defined(_WIN32)
    #include <direct.h>
    #define MAKE_SAVE_DIR(SAVE_DIR) _mkdir(SAVE_DIR)
#else
    #include <sys/stat.h>
    #include <sys/types.h>
    #define MAKE_SAVE_DIR(SAVE_DIR) mkdir(SAVE_DIR, 0777)
#endif

//TODO: Finish this
Memory* memory_init(FILE* rom_file, FILE* boot_rom_file) {
    if (rom_file == NULL) {
        printError("Error opening ROM");
        return NULL;
    }

    Memory* mem = (Memory*)malloc(sizeof(Memory));
    if (mem == NULL) {
        printError("Error freeing space for ROM data");
        return NULL;
    }

    MBC* mbc_chip = mbc_init(get_mbc_data(rom_file));
    
    //If MBC Chip is NULL, can't do much so return
    if (mbc_chip == NULL) {
        free(mem);
        return NULL;
    }

    mem->mbc_chip = mbc_chip;

    //Setup Memory Arrays
    //Fixed memory locations

    mem->vram_0 = (uint8_t*)calloc(0x2000, 1); //8kb vram bank. CGB has a second vram bank.
    mem->wram_x = (uint8_t*)calloc(0x2000, 1); //DMG has 2 fixed banks, CBG has 1 fixed and 7 switchable
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

    //Load ROM data
    load_rom_data(mem, rom_file);

    //Gets game name
    get_game_name(mem);

    //Loads save data if it exists
    if (mbc_chip->has_battery) { load_save_data(mem); } //Load save data if save data exists and game supports it

    //Check for boot rom
    //TODO: Boot ROM is currently very buggy and I'm not sure why?
    
    //If boot rom is NULL, leave it unmapped
    if (boot_rom_file == NULL) {
        mem->boot_rom_size = 0;
        mem->boot_rom = NULL;
        mem->local_state.boot_rom_mapped = 0;
    }
    else {
        //Load boot ROM. If load fails, free the space it was allocating and keep it disabled
        if (load_boot_rom_data(mem, boot_rom_file)) {
            if (mem->boot_rom != NULL) 
                free(mem->boot_rom);

            mem->boot_rom_size = 0;
            mem->local_state.boot_rom_mapped = 0;
        }
    }

    //Start with no buttons pressed
    mem->local_state.button_state = 0x0F;
    mem->local_state.dpad_state = 0x0F;

    //If there was an error in initializing any required memory, destroy memory struct and return NULL
    if (mem->vram_0 == NULL || mem->wram_x == NULL || mem->oam == NULL || mem->io == NULL || mem->hram == NULL ||
        mem->rom_x == NULL) {
        memory_destroy(mem);
        return NULL;
    }

    return mem;
}

//Frees memory that memory struct owns
void memory_destroy(Memory* mem) {
    if (mem == NULL)
        return;

    if (mem->mbc_chip != NULL) { mbc_destroy(mem->mbc_chip); }
    if (mem->vram_0 != NULL) { free(mem->vram_0); }
    if (mem->wram_x != NULL) { free(mem->wram_x); }
    if (mem->oam != NULL) { free(mem->oam); }
    if (mem->io != NULL) { free(mem->io); }
    if (mem->hram != NULL) { free(mem->hram); }
    if (mem->rom_x != NULL) { free(mem->rom_x); }
    if (mem->exram_x != NULL) { free(mem->exram_x); }
    if (mem->boot_rom != NULL) { free(mem->boot_rom); }

    free(mem);
}

//Gets data for MBC initialization
MBC_Data* get_mbc_data(FILE* rom_file) {
    if (rom_file == NULL)
        return NULL;

    //Get MBC chip data
    //Gets intial buffer sizes and MBC type
    uint8_t mbc_type;
    uint8_t rom_byte;
    uint8_t ram_byte;

    //Get MBC, ROM, and RAM bytes and return if it can't be found
    //MBC Type byte
    fseek(rom_file, 0x147, SEEK_SET);
    if (fread(&mbc_type, 1, 1, rom_file) == 0) {
        rewind(rom_file); //Reset file pointer...
        return NULL;
    }

    //MBC Rom byte
    fseek(rom_file, 0x148, SEEK_SET);
    if (fread(&rom_byte, 1, 1, rom_file) == 0) {
        rewind(rom_file); //Reset file pointer...
        return NULL;
    }

    //MBC Ram byte
    fseek(rom_file, 0x149, SEEK_SET);
    if (fread(&ram_byte, 1, 1, rom_file) == 0) {
        rewind(rom_file); //Reset file pointer...
        return NULL;
    }

    //Get pointer for data
    MBC_Data* data = (MBC_Data*)malloc(sizeof(MBC_Data));
    if (data == NULL) {
        rewind(rom_file); //Reset file pointer...
        return NULL;
    }

    data->mbc_type_byte = mbc_type;
    data->mbc_rom_size_byte = rom_byte;
    data->mbc_ram_size_byte = ram_byte;

    rewind(rom_file); //Reset file pointer...
    return data;
}

//Load ROM Data
//Return 0 for success, 1 if failure
uint8_t load_rom_data(Memory* mem, FILE* rom_file) {
    if (rom_file == NULL)
        return 1;

    //Load ROM...
    fseek(rom_file, 0, SEEK_END);
    long num_bytes = ftell(rom_file);
    rewind(rom_file);

    if (!fread(mem->rom_x, 1, num_bytes, rom_file)) {
        printError("ROM file too small!");
        return 1;
    }

    return 0;
}

//Load save data
//Return 0 for success, 1 for failure
uint8_t load_save_data(Memory* mem) {
    //Load save file (if it exists)
    char file_path[100] = "saves/";
    strcat(file_path, mem->game_name);
    strcat(file_path, ".sav");
    FILE* save_data = fopen(file_path, "rb");

    if (save_data != NULL && mem->mbc_chip->has_battery && mem->exram_x != NULL) {
        //Get buffer size
        fseek(save_data, 0, SEEK_END);
        long size = ftell(save_data);
        rewind(save_data);

        if (!fread(mem->exram_x, 1, size, save_data)) {
            fclose(save_data);
            return 1;
        }

        fclose(save_data);
    }
    else
        return 1;
}

//Saves save data
void save_save_data(Memory* mem) {
    //Get file path
    char file_path[100] = "saves/";
    strcat(file_path, mem->game_name);
    strcat(file_path, ".sav");

    FILE* save = fopen(file_path, "wb");

    //If save is NULL, then save directory doesn't exist yet, so create it here
    if (save == NULL) {
        MAKE_SAVE_DIR("saves");
        save = fopen(file_path, "wb"); //Try to open file again. If that fails, no save data :(
    }

    if (save != NULL && mem->mbc_chip->has_battery && mem->exram_x != NULL) {
        fwrite((void*)mem->exram_x, 1, mem->mbc_chip->num_exram_banks * 0x2000, save);

        fclose(save);
    }
}

//Load boot ROM
//Return 0 for success, 1 for failure
uint8_t load_boot_rom_data(Memory* mem, FILE* boot_rom_file) {
    if (boot_rom_file == NULL)
        return 1;

    //Load boot ROM...
    fseek(boot_rom_file, 0, SEEK_END);
    long num_bytes = ftell(boot_rom_file);
    rewind(boot_rom_file);

    //Create buffer for boot ROM
    mem->boot_rom = (uint8_t*)malloc(num_bytes);

    if (!fread(mem->boot_rom, 1, num_bytes, boot_rom_file)) {
        printError("ROM file too small!");
        return 1;
    }

    //Set boot rom flags
    mem->boot_rom_size = num_bytes;
    mem->local_state.boot_rom_mapped = 1;

    return 0;
}

//Gets and stores game name
void get_game_name(Memory* mem) {
    //Game name is stored in bytes 0x134-0x143 in the cartridge header
    
    //If there is no ROM loaded, then no game will run anyway
    if (mem->rom_x == NULL)
        return;

    //Allocate space. Game name will only be max 16 characters
    //It is NULL terminated
    mem->game_name = (char*)calloc(16, 1);

    int mem_index = 0x134;
    int name_index = 0;
    while (mem_index <= 0x143 && mem->rom_x[mem_index] != '\0')
        mem->game_name[name_index++] = mem->rom_x[mem_index++];
}

//Returns memory information for accessed address
MemoryValue get_memory_value(Memory* mem, uint16_t address) {
    uint8_t* ptr = NULL;
    MemoryRange range = RANGE_PROHIBITED; //Default to avoid weird errors

    //Get correct values based on range
    if (address < 0x8000) {
        range = RANGE_ROM;
        ptr = get_rom_ptr(mem, address); //ROM area
    }

    else if (address < 0xA000) {
        range = RANGE_VRAM; //VRAM area
        ptr = get_vram_ptr(mem, address); //VRAM area
    }

    else if (address < 0xC000) {
        range = RANGE_EXRAM; //EXRAM area
        ptr = get_exram_ptr(mem, address);
    }

    else if (address < 0xE000) {
        range = RANGE_WRAM;
        ptr = get_wram_ptr(mem, address); //WRAM area
    }

    else if (address < 0xFE00) {
        range = RANGE_WRAM;
        ptr = get_wram_ptr(mem, address - 0x2000); //Echo RAM mirrors WRAM, so take the WRAM value and just offset it more
    }

    else if (address < 0xFEA0) {
        range = RANGE_OAM;
        ptr = &mem->oam[address - 0xFE00]; //OAM area
    }

    else if (address < 0xFF00) {
        range = RANGE_PROHIBITED;
        ptr = NULL;
    }

    else if (address < 0xFF80) {
        range = RANGE_IO;
        ptr = &mem->io[address - 0xFF00];
    }
    
    else if (address <= 0xFFFF) {
        range = RANGE_HRAM;
        ptr = &mem->hram[address - 0xFF80];
    }

    MemoryValue result = (MemoryValue){ .mem_ptr = ptr, .range = range };

    return result;
}

//Get memory pointer for ROM area
uint8_t* get_rom_ptr(Memory* mem, uint16_t address) {
    //Get index
    uint32_t index;
   
    //If BOOT rom is enabled and reading from it return that
    //TODO: Put boot rom stuff in a better spot
    if (address < mem->boot_rom_size) {
        if (mem->local_state.boot_rom_mapped && mem->boot_rom != NULL) {
            return &mem->boot_rom[address];
        }
    }

    if (address < 0x4000) {
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

    return &mem->rom_x[index];
}

//Get memory pointer from VRAM area
uint8_t* get_vram_ptr(Memory* mem, uint16_t address) {
    //Get index
    uint32_t index = address - 0x8000;

    return &mem->vram_0[index]; //Return VRAM bank 0
}

//Get memory pointer from EXRAM area
uint8_t* get_exram_ptr(Memory* mem, uint16_t address) {
    //Get index
    uint32_t index;
    if (mem->mbc_chip->mbc_type != MBC_2)
        index = (((address - 0xA000) + (mem->mbc_chip->current_exram_bank * 0x2000))) % (mem->mbc_chip->num_exram_banks * 0x2000);
    else
        index = (address - 0xA000) % 0x200; //MBC2 only has 0x200 bytes for some reason

    return &mem->exram_x[index];
}

//Get memory pointer from WRAM area
uint8_t* get_wram_ptr(Memory* mem, uint16_t address) {
    //If we are in DMG mode, there is only 2 WRAM banks so it is just a simple offset
    return &mem->wram_x[address - 0xC000];
}

//Disables boot rom and frees space if bootrom exists
void disable_bootrom(Memory* mem) {
    if (mem->boot_rom != NULL) { free(mem->boot_rom); }
    mem->local_state.boot_rom_mapped = 0;
}