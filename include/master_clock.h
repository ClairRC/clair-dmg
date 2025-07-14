#ifndef MASTER_CLOCK_H
#define MASTER_CLOCK_H

#include <stdint.h>
#include "memory.h"
#include "ppu.h"
#include "apu.h"

/*
* Master clock takes care of timing, including the system time, the amount of cycles total, and the time 
* of the current frame. This is also directly tied to the timing registers
*/

//Holds flags for timing registers
typedef struct {
    uint8_t prev_tac_bit; //Stores what the last Timer Control bit was for updating TIMA
    uint8_t prev_tma_val; //Stores previous M-Cycle's TMA value for TIMA overflow

    uint8_t tima_overflow; //Flag to check whether TIMA overflowed
    uint8_t tima_overflow_buffer; //Flag to check if TIMA overflowed 2 cycles ago for TIMA updates
} LocalTimerState;

//Holds system timing information
typedef struct {
    //Timer states
    GlobalTimerState* global_state;
    LocalTimerState local_state;
} MasterClock;

MasterClock* master_clock_init(GlobalTimerState* global_state);
void master_clock_destroy(MasterClock* clock);
void update_timing_registers(MasterClock* clock, Memory* mem);

#endif 