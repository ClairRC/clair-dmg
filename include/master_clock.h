#ifndef MASTER_CLOCK_H
#define MASTER_CLOCK_H

#include <stdint.h>
#include "memory.h"
#include "ppu.h"

//Holds system timing information

typedef struct {
    uint16_t system_time; //System timer (~4MHz)
    uint32_t frame_time; //Elapsed frame time

    uint8_t prev_tac_bit; //Stores what the last Timer Control bit was for updating TIMA
    uint8_t prev_tma_val; //Stores previous M-Cycle's TMA value for TIMA overflow

    uint8_t tima_overflow; //Flag to check whether TIMA overflowed
    uint8_t tima_overflow_buffer; //Flag to check if TIMA overflowed 2 cycles ago for TIMA updates
} MasterClock;

MasterClock* master_clock_init(); //Initializes master clock
void update_timing_registers(MasterClock* clock, Memory* mem);
void update_dma_transfer(Memory* mem);
void update_ppu(MasterClock* clock, PPU* ppu, uint32_t frame_time);
void update_ppu_state(MasterClock* clock, PPU* ppu, uint32_t frame_time);
void update_stat(PPU* ppu, uint32_t frame_time);

#endif 