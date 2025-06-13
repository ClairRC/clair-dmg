#ifndef SYSTEM_H 
#define SYSTEM_H 

#include "memory.h" 
#include "cpu.h"

//Holds global system information, including system time and pointers to individual pieces
//The point of this is to have like a "central" struct
//for better encapsulation of emulator specific behavior.

//This basically will tie it all together, so 1 System has 1 memory, cpu, ppu, etc.
//This handles initializing all the rest of the pointers as well...

//Soo basically the individual pieces are responsible for their own state, but this will
//update them in relation to the emulator state as a whole, if that makes sense... Again I'm mostly thinking of timing.

typedef struct {
    Memory* memory;
    CPU* cpu;

    //Timer Stuff (in t-cycles)
    emu_time elapsed_time; //Total time elapsed since loading ROM
    emu_time frame_time; //Total time elapsed this frame
    emu_time delta_time; //Number of cycles since last update (I think this is always time last instruction took)
} EmulatorSystem;

//Initializes system.
//Only takes memory pointer since memory is the "indepenent" piece of the system 
//that everything else uses
EmulatorSystem* system_init(Memory*); //TODO: Implement

#endif 