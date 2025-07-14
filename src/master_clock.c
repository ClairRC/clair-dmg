#include <stdlib.h>

#include "master_clock.h"
#include "interrupt_handler.h"
#include "hardware_def.h"
#include "ppu.h"
#include "logging.h"

#define GET_BIT(num, bit) ((num) >> (bit)) & 0x1 //Gets value of specific bit (starting at 0)

//Useful bitwise definitions
#define TAC_CLOCK_SELECT 0x3 //Bottom 2 bits select TAC clock value
#define TAC_ENABLE 0x4 //Bit 2 is TAC enable

//Iniital values for system clock
MasterClock* master_clock_init(GlobalTimerState* global_state) {
    if (global_state == NULL) {
        printError("Unable to initialize system clock.");
        return NULL;
    }

	MasterClock* sys_clock = (MasterClock*)malloc(sizeof(MasterClock));

    if (sys_clock == NULL) {
        printError("Unable to initialize system clock.");
        return NULL;
    }

    sys_clock->global_state = global_state;
    
    sys_clock->local_state.prev_tac_bit = 0; //Stores what the last Timer Control bit was for updating TIMA
    sys_clock->local_state.prev_tma_val = 0; //Stores previous M-Cycle's TMA value for TIMA overflow
    sys_clock->local_state.tima_overflow = 0; //Flag to check whether TIMA overflowed
    sys_clock->local_state.tima_overflow_buffer = 0; //Flag to check if TIMA overflowed 2 cycles ago for TIMA updates

    return sys_clock;
}

void master_clock_destroy(MasterClock* clock) {
    if (clock != NULL)
        free(clock);
}

//Updates timer registers
void update_timing_registers(MasterClock* clock, Memory* mem) {
    uint16_t start_time = clock->global_state->system_time;

    //DIV reflects bottom 8 bits of system time
    uint8_t div = (uint8_t)((start_time) >> 8);
    mem->DIV_LOCATION = div; //0xFF04 = DIV

    //Update TIMA
    //Updates when specific bit in DIV goes from 1 to 0. The bit is specified by TAC
    uint8_t tac_value = mem->TAC_LOCATION; //TAC at 0xFF07
    uint8_t tac_bit_pos = 0; //Which bit of system clock is being checked
    uint8_t tac_bit_select = (tac_value & TAC_CLOCK_SELECT);

    //Gets bit that TIMA is looking for
    if (tac_bit_select == 0x0)
        tac_bit_pos = 9; //Increments every (2^9) * 2 t-cycles
    else if (tac_bit_select == 0x01)
        tac_bit_pos = 3; //Increments every (2^3) * 2 t-cycles
    else if (tac_bit_select == 0x02)
        tac_bit_pos = 5; //Increments every (2^5) * 2 t-cycles
    else if (tac_bit_select == 0x03)
        tac_bit_pos = 7; //Increments every (2^7) * 2 t-cycles

    uint8_t tac_bit = GET_BIT(start_time, tac_bit_pos);
    uint8_t prev_tac_bit = clock->local_state.prev_tac_bit; //Previous value of TAC

    //If TIMA overflowed on the previous cycle, request the interrupt now
    if (clock->local_state.tima_overflow) {
        clock->local_state.tima_overflow = 0;
        //If TIMA gets written to, the overflow gets ignored
        if (mem->TIMA_LOCATION == 0) {
            clock->local_state.tima_overflow_buffer = 1;
            requestInterrupt(INTERRUPT_TIMER, mem);
            mem->TIMA_LOCATION = mem->TMA_LOCATION; //Resest TIMA to TMA
        }
    }

    //If bit went from 1 to 0 and TIMA is enabled..
    if (prev_tac_bit == 1 && tac_bit == 0 && GET_BIT(tac_value, 2)) {
        ++mem->TIMA_LOCATION; //Increment TIMA (0xFF05)

        //If there was an overflow...
        if (mem->TIMA_LOCATION == 0x00) {
            //Sets flag that TIMA overflowed. This means that TIMA remains 0
            //during this m-cycle (correct behavior!!)
            clock->local_state.tima_overflow = 1;
        }
    }

    //Update previous TAC bit value
    clock->local_state.prev_tac_bit = tac_bit;
}

//Updates state of the APU
//TODO: Find a better home
/*
void update_apu_state(MasterClock* clock, APU* apu) {
    //"DIV-APU" updates when bit 4 of DIV goes from 1 to 0
    //TODO: In CGB double speed mode, this uses bit 5 instead
    uint8_t current_div_bit = apu->memory->DIV_LOCATION & (1 << 4);

    //Update APU Timers
    if (apu->prev_div_bit == 1 && current_div_bit == 0) {
        apu->prev_div_apu = apu->div_apu++;
    }


    //Activate Channels if necessary

    /*
     * CH1 State
     *

    //Get CH1 DAC State
    uint8_t ch1_dac_active = apu->memory->NR12_LOCATION & 0xF8;

    //If DAC is inactive, then channel is inactive
    if (!ch1_dac_active)
        apu->ch1.active = 0;

    //If channel was triggered, wasn't active, and DAC is active, activate the channel
    if (apu->memory->state.ch1_trigger == 1) {
        apu->memory->state.ch1_trigger = 0;

        if (ch1_dac_active && !apu->ch1.active) 
            activate_ch1(apu, clock->elapsed_time);
    }

    /*
     * CH2 State
     *

     //Get CH2 DAC State
    uint8_t ch2_dac_active = apu->memory->NR22_LOCATION & 0xF8;

    //If DAC is inactive, then channel is inactive
    if (!ch2_dac_active)
        apu->ch2.active = 0;

    //If channel was triggered, wasn't active, and DAC is active, activate the channel
    if (apu->memory->state.ch2_trigger == 1) {
        apu->memory->state.ch2_trigger = 0;

        if (ch2_dac_active && !apu->ch2.active)
            activate_ch2(apu, clock->elapsed_time);
    }

    /*
     * CH3 State
     *

    //Get CH3 DAC State
    uint8_t ch3_dac_active = (apu->memory->NR30_LOCATION & 7) ? 1 : 0;

    if (!ch3_dac_active)
        apu->ch3.active = 0;

    //If channel was triggered, wasn't active, and DAC is active, activate the channel
    if (apu->memory->state.ch3_trigger == 1) {
        apu->memory->state.ch3_trigger = 0;

        if (ch3_dac_active && !apu->ch3.active)
            activate_ch3(apu, clock->elapsed_time);
    }

    /*
     * CH4 State
     *

    //Get CH4 DAC State
    uint8_t ch4_dac_active = apu->memory->NR42_LOCATION & 0xF8;

    if (!ch4_dac_active)
        apu->ch4.active = 0;

    //If channel was triggered, wasn't active, and DAC is active, activate the channel
    if (apu->memory->state.ch4_trigger == 1) {
        apu->memory->state.ch4_trigger = 0;

        if (ch4_dac_active && !apu->ch4.active)
            activate_ch4(apu, clock->elapsed_time);
    }
}*/
