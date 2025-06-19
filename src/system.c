#include <stdlib.h>
#include "system.h" 
#include "logging.h"

EmulatorSystem* system_init(Memory* memory) {
    EmulatorSystem* system = malloc(sizeof(EmulatorSystem));
    if (system == NULL) {
        printError("Error intializing Emulator...");
        free(memory);
        return NULL;
    }

    CPU* cpu = cpu_init(memory);
    if (cpu == NULL) {
        printError("Error intializing CPU...");
        free(memory);
        free(system);
        return NULL;
    }

    //TODO: Make tihs better.
    PPU* ppu = ppu_init(memory);

    SystemClock* sys_clock = (SystemClock*)malloc(sizeof(SystemClock));

    if (sys_clock == NULL) {
        printError("Error intializing System Clock...");
        free(system);
        free(cpu);
        free(memory);
        return NULL;
    }

    sys_clock->system_time = 0;
    sys_clock->frame_time = 0;
    sys_clock->delta_time = 0;
    sys_clock->prev_tac_bit = 0;
    sys_clock->prev_tma_val = 0;
    sys_clock->tima_overflow = 0;
    sys_clock->tima_overflow_buffer = 0;

    system->memory = memory;
    system->cpu = cpu;
    system->ppu = ppu;
    system->system_clock = sys_clock;

    return system;
}