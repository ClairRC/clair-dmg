#include <stdio.h>
#include <stdlib.h>
#include "init.h" 
#include "logging.h"
#include "fe_de_ex.h"
#include "instructions.h"
#include "hardware_registers.h"
#include "sdl_data.h"

#define GAME_NAME "game.gb"
#define BOOTROM_DIR "boot.bin"

int emulator_init() {
    init_opcodes();
    init_hw_registers();

    //SDL display data
    SDL_Data* sdl_data = sdl_init(160, 144); //Width and height of Gameboy display
    if (sdl_data == NULL)
        return 1;

    //Open ROM file
    //Open save file if it exists
    FILE* rom_file = fopen(GAME_NAME, "rb");
    FILE* boot_rom_file = fopen(BOOTROM_DIR, "rb");

    //Emulator System initialization
    EmulatorSystem* system = system_init(rom_file, boot_rom_file, sdl_data);

    //Close files
    if (rom_file != NULL)
        fclose(rom_file);
    if (boot_rom_file != NULL)
        fclose(boot_rom_file);

    //Check for errors involving emulator initialization
    if (system == NULL)
        return 1;

    //Sets window title to be game name
    change_window_name(sdl_data, system->memory->game_name);

    //If boot rom was not loaded, load initial CPU values manully
    if (system->memory->boot_rom == NULL)
        init_cpu_vals(system); 

    //Begin instruction loop!
    int success = fe_de_ex(system);

    //Delete SDL stuff after instruction loop
    if (sdl_data != NULL)
        sdl_destroy(sdl_data);

    return success;
}

//Initializes CPU and Memory values if bootrom is missing
int init_cpu_vals(EmulatorSystem* system) {
    //DMG CPU Values
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


    //TODO: Get rid of magic numbers
    system->memory->io[0x0] = 0xFF;
    system->memory->io[0x05] = 0x0;
    system->memory->io[0x06] = 0x0;
    system->memory->io[0x07] = 0x0;
    system->memory->io[0x40] = 0x91;
    system->memory->io[0x44] = 0x0;
    system->memory->io[0x47] = 0xFC;

    return 0;
}