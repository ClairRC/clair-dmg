#include <stdlib.h>
#include "system.h" 
#include "system_state.h"
#include "logging.h"

EmulatorSystem* system_init(FILE* rom_file, FILE* save_data, FILE* boot_rom_file, SDL_Data* sdl_data) {
    //ROM and SDL information required for emulator to run
    if (rom_file == NULL || sdl_data == NULL) {
        printError("Unable to open ROM");
        return NULL;
    }

    EmulatorSystem* system = malloc(sizeof(EmulatorSystem));
    if (system == NULL) {
        printError("Unable to open ROM");
        return NULL;
    }

    //SDL Data
    system->sdl_data = sdl_data;

    //Memory and System State
    system->memory = memory_init(rom_file, save_data, NULL);
    system->system_state = system_state_init();
    system->bus = memory_bus_init(system->memory, system->system_state);
    
    //Subsystems
    system->cpu = cpu_init(system->bus);
    system->ppu = ppu_init(system->bus, system->system_state->ppu_state, sdl_data->display_data);
    system->sys_clock = master_clock_init(system->system_state->timer_state);
    APU* apu = NULL; //FIX!!!

    //If required systems are NULL, destroy system and return NULL
    if (system->memory == NULL || system->system_state == NULL || system->bus == NULL ||
        system->cpu == NULL || system->ppu == NULL || system->sys_clock == NULL /*|| system->apu == NULL */ ) {
        system_destroy(system);
        return NULL;
    }

    return system;
}

void system_destroy(EmulatorSystem* system) {
    if (system == NULL)
        return;
    
    //Destroys each the pointers it owns
    if (system->bus != NULL) { memory_bus_destroy(system->bus); }
    if (system->memory != NULL) { memory_destroy(system->memory); }
    if (system->system_state != NULL) { system_state_destroy(system->system_state); }
    if (system->cpu != NULL) { cpu_destroy(system->cpu); }
    if (system->ppu != NULL) { ppu_destroy(system->ppu); }
    if (system->apu != NULL) { apu_destroy(system->apu); }
    if (system->sys_clock != NULL) { master_clock_destroy(system->sys_clock); }

    free(system); //Free itself
}

//Updates timing of different hardware
void tick_hardware(EmulatorSystem* system, uint16_t ticks) {
    uint16_t elapsed_ticks = 0;

    while (elapsed_ticks < ticks) {
        update_timing_registers(system->sys_clock, system->memory); //Update timer registers
        update_dma_transfer(system); //Handle DMA transfer if active
        update_ppu(system->ppu);

        //TODO: Resvole APU Stuff
        /*update_apu_state(system->sys_clock, system->apu);
        update_apu(system->apu, system->sys_clock->elapsed_time);
        */

        //If frame just ends, poll SDL to update input and check if the emulator is closed
        if (system->system_state->ppu_state->frame_time == 0) {
            if (poll_events(system->sdl_data->input_data)) {
                system->system_state->running = 0; //If SDL is quit, stop running emulator
            }
            
            //Update memory state to reflect current button state
            system->memory->local_state.button_state = system->sdl_data->input_data->button_state;
            system->memory->local_state.dpad_state = system->sdl_data->input_data->dpad_state;
        }

        //Add to system time and frame time and update elapsed_ticks
        ++system->sys_clock->global_state->system_time;
        ++system->system_state->ppu_state->frame_time;
        ++system->sys_clock->global_state->elapsed_time;

        ++elapsed_ticks;
    }
}

//Updates DMA
void update_dma_transfer(EmulatorSystem* system) {
    //Wasn't really sure where to put this since DMA is just a memory transfer
    //It didn't feel like it belongs in memory, and it doesn't feel like DMA needs its own module, 
    //so I'm just going to keep it here for now since it is technically a coordinate of different subsystems
    if (!system->system_state->dma_state->active)
        return;

    //Decrement remaining cycles, this allows for 160 to not be included and 0 to be included, which is accurate start/end timing
    --system->system_state->dma_state->remaining_cycles;

    //DMA transfer transfers from 0xXX00-0xXX9F to 0xFE00 to 0xFE9F
    uint16_t remaining_cycles = system->system_state->dma_state->remaining_cycles;
    
    //1 transfer is completed every 4 ticks (each m cycle)
    //Only do the write if we are below the "start" point
    if (remaining_cycles % 4 == 0) {
        //DMA works like echo RAM
        //So I am simulating this by just putting the offset into 0xDE range instead
        if (system->system_state->dma_state->source >= 0xFE) {
            system->system_state->dma_state->source -= 0xFE;
            system->system_state->dma_state->source += 0xDE;
        }

        uint16_t src_address = 
            ((uint16_t)(system->system_state->dma_state->source << 8) | 0x00) + ((640 - remaining_cycles) / 4) - 1; //Divide by 4 since each write happens every 4 ticks
        uint16_t dest_address = 0xFE00 + (640 - remaining_cycles) / 4 - 1;

        uint8_t val = mem_read(system->bus, src_address, DMA_ACCESS);
        mem_write(system->bus, dest_address, val, DMA_ACCESS);
    }

    //If remaining cycles is 0 (DMA transfer just finished), reset it
    if (system->system_state->dma_state->remaining_cycles == 0) {
        system->system_state->dma_state->remaining_cycles = 640;
        system->system_state->dma_state->active = 0;
    }
}