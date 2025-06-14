#include "hardware.h"
#include "interrupt_handler.h"

//Gets value of specific bit (starting at 0)
#define GET_BIT(num, bit) ((num) >> (bit)) & 0x1


//Updates system time and other hardware states based on time passed
void update_hardware(EmulatorSystem* system, uint16_t delta_time) {
    //I think that having this here is good because it makes it so that 
    //updating time passed ALWAYS accounts for specific hardware timing, which is
    //how it works in reality. Plus, it means the instruciton loop should only ever
    //update hardware atomically, which is also accurate to real behavior

    //If DIV was written to, reset system clock
    if (system->memory->io[0x04] == 0x0)
        system->system_clock = 0x0;

    system->system_clock->delta_time = delta_time; //Update delta time

    update_timer_registers(system);

    //Update system clock after hardware has been updated
    system->system_clock->system_time += delta_time;
    system->system_clock->frame_time += delta_time;
}

//Takes emulator system pointer
void update_timer_registers(EmulatorSystem* system) {
    /*
    * This system is kind of weird, but basically the way
    * This works is everything here gets updated before instruction of the cycle
    * So over the course of 3 M-cycles,
    * Update DIV each time 
    * Check if TIMA overflowed 2 cycles ago (writes during the previous cycle would be ignored)
    * Check if TIMA overflowed LAST cycle (overflow ignored if so)
    * If TIMA overflowed last cycle (and was not written to), set flag and request interrupt
    * Check if TIMA overflows THIS cycle (sets flags and resets TIMA to 0)
    *
    * This also keeps track of previous TAC bit to check if TIMA gets updated
    * since it gets updated on a falling edge (regardless of writes/timer reset).
    */

    uint16_t start_time = system->system_clock->system_time;

    //i+=4 because system time never gets desynced from m-cycles, so this just
    //reduces the amount of iterations by a factor of 4.
    //TODO: Check if this would mess things up for mid-instruction updates (if they would ever be necessary?)
    for (int i = 0; i < system->system_clock->delta_time; i+=4) {
        //Set DIV register (upper 8 bits of system time)
        uint8_t div = (uint8_t)((start_time + i) >> 8);
        system->memory->io[0x04] = div; //0xFF04 = DIV

        //Update TIMA
        //Updates when specific bit in DIV goes from 1 to 0. The bit is specified by TAC
        uint8_t tac_value = system->memory->io[0x07]; //TAC at 0xFF07
        uint8_t tac_bit_pos;

        //Gets bit that TIMA is looking for
        if ((tac_value & 0x03) == 0x0)
            tac_bit_pos = 9; //Increments every (2^9) * 2 t-cycles
        else if ((tac_value & 0x03) == 0x01)
            tac_bit_pos = 3; //Increments every (2^3) * 2 t-cycles
        else if ((tac_value & 0x03) == 0x02)
            tac_bit_pos = 5; //Increments every (2^5) * 2 t-cycles
        else if ((tac_value & 0x03) == 0x03)
            tac_bit_pos = 7; //Increments every (2^7) * 2 t-cycles

        uint8_t tac_bit = GET_BIT(system->system_clock->system_time + i, tac_bit_pos);
        uint8_t prev_tac_bit = system->system_clock->prev_tac_bit;

        //TODO: Figure out what to do if TIMA overflowed 2 cycles ago but then gets 
        //written over/TMA gets written over...

        //If TIMA overflowed on the previous m-cycle, request the interrupt now
        if ((start_time + i) % 4 == 0 && system->system_clock->tima_overflow) {
            system->system_clock->tima_overflow = 0;
            //If TIMA gets written to, the overflow gets ignored
            if (system->memory->io[0x05] == 0) {
                system->system_clock->tima_overflow_buffer = 1;
                requestInterrupt(INTERRUPT_TIMER, system->memory);
                system->memory->io[0x05] = system->memory->io[0x06]; //Resest TIMA to TMA
            }
        }

        //If bit went from 1 to 0 and TIMA is enabled..
        if (prev_tac_bit == 1 && tac_bit == 0 && GET_BIT(tac_value, 2)) {
            ++system->memory->io[0x05]; //Increment TIMA (0xFF05)
            
            //If there was an overflow...
            if (system->memory->io[0x05] == 0x00) {
                //Sets flag that TIMA overflowed. This means that TIMA remains 0
                //during this m-cycle (correct behavior!!)
                system->system_clock->tima_overflow = 1;
            }
        }

        //Update previous TAC bit value
        system->system_clock->prev_tac_bit = tac_bit;
    }
}