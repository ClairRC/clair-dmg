#include <stdio.h>
#include <stdlib.h>
#include "init.h" 
#include "logging.h"
#include "fe_de_ex.h"
#include "instructions.h"
#include "hardware_registers.h"
#include "game_display.h"

#define GAME_NAME "C:/Users/Claire/Desktop/tet.gb"
#define BOOTROM_DIR "C:/Users/Claire/Desktop/boot.bi"

int emulator_init() {
    init_opcodes();
    init_hw_registers();
    Memory* mem = load_rom_data();

    if (mem == NULL)
        return 1;

    //SDL display data
    DisplayData* display = sdl_init(160, 144); //Width and height of Gameboy display

    if (display == NULL) {
        printError("Display is NULL");
        return 1;
    }

    EmulatorSystem* system = system_init(mem, display);

    if (system == NULL)
        return 1;

    //Finally, if boot rom was not loaded, load initial CPU values manully
    if (mem->boot_rom == NULL)
        init_cpu_vals(system);

    //Begin instruction loop!
    int success = fe_de_ex(system);

    return success;
}

Memory* load_rom_data() {
    //For now I will hardcode the file path....
    char* file_path = GAME_NAME; 
    FILE* file_ptr = fopen(file_path, "rb");

    //Fail to open file
    if (file_ptr == NULL) {
        printError("Failed to open ROM file...");
        fclose(file_ptr);
        return NULL;
    }

    //Gets intial buffer sizes and MBC type
    uint8_t mbc_type;
    uint8_t rom_byte;
    uint8_t ram_byte;

    //Get MBC, ROM, and RAM bytes and return if it can't be found
    fseek(file_ptr, 0x147, SEEK_SET);
    if (fread(&mbc_type, 1, 1, file_ptr) == 0) {
        printError("Failed to load ROM file...");
        fclose(file_ptr);
        return NULL; 
    }

    fseek(file_ptr, 0x148, SEEK_SET);
    if (fread(&rom_byte, 1, 1, file_ptr) == 0) {
        printError("Failed to load ROM file...");
        fclose(file_ptr);
        return NULL;
    }

    fseek(file_ptr, 0x149, SEEK_SET);
    if (fread(&ram_byte, 1, 1, file_ptr) == 0) {
        fclose(file_ptr);
        printError("Failed to load ROM file...");
        return NULL;
    }

    //mbc_type &= 0x0F;
    rewind(file_ptr); //Reset file pointer...
    
    Memory* mem = memory_init(mbc_type, rom_byte, ram_byte);

    printf("MBC TYPE BYTE: %02X\nMBC TYPE:%d\nROM BYTE: %02X\nROM BANKS: %d\nRAM BYTE: %02X\nRAM BANKS: %d\nMODE SWITCH: %d\n",
        mbc_type, mem->mbc_chip->mbc_type,
        mem->mbc_chip->mbc_rom_size_byte, mem->mbc_chip->num_rom_banks,
        mem->mbc_chip->mbc_ram_size_byte, mem->mbc_chip->num_exram_banks, mem->mbc_chip->has_mode_switch);

    //Load ROM...
    fseek(file_ptr, 0, SEEK_END);
    long num_bytes = ftell(file_ptr);
    rewind(file_ptr);

    if (!fread(mem->rom_x, 1, num_bytes, file_ptr)) {
        printError("ROM file too small!");
        free(mem);
        fclose(file_ptr);
        return NULL;
    }

    fclose(file_ptr);

    //Check for boot rom
    FILE* boot_ptr = fopen(BOOTROM_DIR, "rb");
    if (boot_ptr != NULL) {
        fseek(boot_ptr, 0, SEEK_END);
        unsigned int boot_rom_size = ftell(boot_ptr);
        rewind(boot_ptr);

        uint8_t* boot_rom = (uint8_t*)malloc(boot_rom_size);

        //If bootrom is not null, load it
        if (boot_rom == NULL) {
            printError("Error loading boot ROM, skipping...");
        }
        else {
            if (!fread(boot_rom, 1, boot_rom_size, boot_ptr)) {
                printError("Error loading boot ROM, skipping...");
                free(boot_rom); //Free boot rom memory if it can't get loaded into...
            }
            else {
                mem->boot_rom = boot_rom;
                mem->boot_rom_size = boot_rom_size; //In bytes
                mem->state.boot_rom_mapped = 1;
            }
        }
    }
    else
        printError("Error loading boot ROM, skipping...");

    //Finally, close file pointer if its not NULL
    if (boot_ptr != NULL) { fclose(boot_ptr); }

    //Return memory pointer
    return mem;
}

int init_cpu_vals(EmulatorSystem* system) {
    system->cpu->registers.pc = 0x100; //Skip boot ROM
    system->cpu->registers.sp = 0xFFFE; //Set stack pointer....

    system->cpu->registers.A = 0x01;
    system->cpu->registers.F = 0xB0;
    system->cpu->registers.B = 0x00;
    system->cpu->registers.C = 0x13;
    system->cpu->registers.D = 0x00;
    system->cpu->registers.E = 0xD8;
    system->cpu->registers.H = 0x01;
    system->cpu->registers.L = 0x4D;
    system->cpu->registers.A = 0x01;

    system->memory->io[0x05] = 0x0;
    system->memory->io[0x06] = 0x0;
    system->memory->io[0x07] = 0x0;
    system->memory->io[0x40] = 0x91;
    system->memory->io[0x44] = 0x0;
    system->memory->io[0x47] = 0xFC;

    return 0;
}