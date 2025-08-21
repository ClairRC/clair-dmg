#ifndef SYSTEM_H 
#define SYSTEM_H 

#include "memory_bus.h" 
#include "system_state.h"
#include "cpu.h"
#include "ppu.h"
#include "master_clock.h"
#include "apu.h"

//Holds global system information, including system time and pointers to individual pieces
//The point of this is to have like a "central" struct
//for better encapsulation of emulator specific behavior.

//This basically will tie it all together, so 1 System has 1 memory, cpu, ppu, etc.
//This handles initializing all the rest of the pointers as well...

//Soo basically the individual pieces are responsible for their own state, but this will
//update them in relation to the emulator state as a whole, if that makes sense... Again I'm mostly thinking of timing.

typedef struct {
    //SDL Data reference
    SDL_Data* sdl_data;

    //Memory
    MemoryBus* bus;
    Memory* memory;

    //System states
    GlobalSystemState* system_state;

    //Subsystems
    CPU* cpu;
    PPU* ppu;
    APU* apu;
    MasterClock* sys_clock;
} EmulatorSystem;

//Initializes system.
//Only takes memory pointer since memory is the "indepenent" piece of the system 
//that everything else uses
EmulatorSystem* system_init(FILE* rom_file, FILE* boot_rom_file, SDL_Data* sdl_data);
void system_destroy(EmulatorSystem* system);
void tick_hardware(EmulatorSystem* system, uint16_t ticks); //Updates hardware timing
void update_dma_transfer(EmulatorSystem* system);

#endif 