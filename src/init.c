#include <stdio.h>
#include <stdlib.h>
#include "init.h" 
#include "logging.h"
#include "fe_de_ex.h"
#include "instructions.h"
#include "hardware_registers.h"

int emulator_init() {
    init_opcodes();
    init_hw_registers();
    Memory* mem = load_rom_data();

    if (mem == NULL)
        return 1;

    EmulatorSystem* system = system_init(mem);

    if (system == NULL)
        return 1;

    init_cpu_vals(system);

    //Begin instruction loop!
    int success = fe_de_ex(system);

    return success;
}

Memory* load_rom_data() {
    //For now I will hardcode the file path....
    char* file_path = "test.gb"; 
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

    mbc_type &= 0x0F;
    rewind(file_ptr); //Reset file pointer...
    
    Memory* mem = memory_init(mbc_type, rom_byte, ram_byte);
    
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

    //Return memory pointer
    return mem;
}

int init_cpu_vals(EmulatorSystem* system) {
    system->cpu->registers.pc = 0xFF; //Skip boot ROM
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

    return 0;
}