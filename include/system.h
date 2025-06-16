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
    //Timer Stuff (in t-cycles)
    uint16_t system_time; //System timer (~4MHz)
    uint32_t frame_time; //Total time elapsed this frame
    uint16_t delta_time; //Number of cycles since last update (I think this is always time last instruction took)

    uint8_t prev_tac_bit; //Stores what the last Timer Control bit was for updating TIMA
    uint8_t prev_tma_val; //Stores previous M-Cycle's TMA value for TIMA overflow

    uint8_t tima_overflow; //Flag to check whether TIMA overflowed
    uint8_t tima_overflow_buffer; //Flag to check if TIMA overflowed 2 cycles ago for TIMA updates
} SystemClock;

typedef struct {
    Memory* memory;
    CPU* cpu;
    SystemClock* system_clock;
} EmulatorSystem;

//Initializes system.
//Only takes memory pointer since memory is the "indepenent" piece of the system 
//that everything else uses
EmulatorSystem* system_init(Memory*);

#endif 