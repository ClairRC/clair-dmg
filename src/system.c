#include <stdlib.h>
#include "system.h" 
#include "logging.h"

EmulatorSystem* system_init(Memory* memory, SDL_Data* display) {
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
    PPU* ppu = ppu_init(memory, display);

    MasterClock* sys_clock = master_clock_init();

    if (sys_clock == NULL) {
        free(system);
        free(cpu);
        free(memory);
        return NULL;
    }

    system->memory = memory;
    system->cpu = cpu;
    system->ppu = ppu;
    system->sys_clock = sys_clock;

    return system;
}

//Updates timing of different hardware
void tick_hardware(EmulatorSystem* system, uint16_t ticks) {
    uint16_t elapsed_ticks = 0;

    while (elapsed_ticks < ticks) {
        //Reset system clock if DIV was written to
        if (system->memory->state.div_reset)
            system->sys_clock->system_time = 0;

        update_timing_registers(system->sys_clock, system->memory); //Update timer registers
        update_dma_transfer(system->memory); //Handle DMA transfer if active
        update_ppu(system->sys_clock, system->ppu, system->sys_clock->frame_time);

        //Add to system time and update elapsed_ticks
        ++system->sys_clock->system_time;
        ++system->sys_clock->frame_time;

        ++elapsed_ticks;
    }
}