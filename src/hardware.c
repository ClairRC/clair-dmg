#include "hardware.h"
#include "interrupt_handler.h"

//Gets value of specific bit (starting at 0)
#define GET_BIT(num, bit) ((num) >> (bit)) & 0x1


/*
* TODO:
* Because cycle timing is pretty important to having the correct writes here, I'm not
* 100% sure the accuracy of this entire module. Since the hardware gets updated
* after the instructions get exectued, then particularly writes that target
* specific addresses COULD throw things off. Additionally, it MIGHT cause DMA transfer
* to begin 1-2 M-cycles early. I am simply not sure, soo uhh I will test this once
* I have a working instruction loop and can run the test roms and see if theres any
* major issues here...
*
* I could defer timing for things like DMA transfer, and delay my writes if
* they get set to a timing register, aaand I'll consider that if it causes issues...
* I suspect that very few games rely on EXACT timings for these things, and as long as I get
* hblank and vblank interrupts correct, then the rest should be rarely buggy at worst.
* Especially given that memory access by the CPU is restricted during DMA access anyway.
*/

//Updates system time and other hardware states based on time passed
void sync_hardware(EmulatorSystem* system, uint16_t delta_time) {
    //I think that having this here is good because it makes it so that 
    //updating time passed ALWAYS accounts for specific hardware timing, which is
    //how it works in reality. Plus, it means the instruciton loop should only ever
    //update hardware atomically, which is also accurate to real behavior

    system->system_clock->delta_time = delta_time; //Update delta time

    update_timer_registers(system); //Update timer registers
    update_dma_transfer(system);
    update_stat_register(system);

    //Update system clock after hardware has been updated
    system->system_clock->system_time += delta_time;
    system->system_clock->frame_time += delta_time;

    //Reset system clock if it was written to
    if (system->memory->div_reset)
        system->system_clock->system_time = 0;

    if (system->system_clock->frame_time >= 70224) {
        system->system_clock->frame_time = 0;
        //This request should be done by the PPU normally, but
        //I am keeping it for testing for now...
        requestInterrupt(INTERRUPT_VBLANK, system->cpu);
    }
}

//Takes emulator system pointer
void update_timer_registers(EmulatorSystem* system) {
    uint16_t start_time = system->system_clock->system_time - system->system_clock->delta_time;

    //i+=4 because system time never gets desynced from m-cycles, so this just
    //reduces the amount of iterations by a factor of 4.
    //TODO: Check if this would mess things up for mid-instruction updates (if they would ever be necessary?)
    for (int i = 0; i < system->system_clock->delta_time; i+=4) {
        //Just here for testing...
        //Increment LY manually to avoid infinite loops
        //TODO: REMOVE THIS
        if ((start_time + i) % 456 == 0) {
            ++system->memory->io[0x44];

            if (system->memory->io[0x44] >= 153) {
                system->memory->io[0x44] = 0;
            }
        }

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
                requestInterrupt(INTERRUPT_TIMER, system->cpu);
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

//Updates DMA, which writes to memory
void update_dma_transfer(EmulatorSystem* system) {
    //If DMA is not active, which is usually true, return immediately
    if (!system->memory->dma_active)
        return;

    //Above I did i+=4 because system time should never desync from m-cycles, 
    //and I probably could do something similar for DMA, but then I have to do the cycles
    //in terms of machine cycles, and I'd rather just stay consistent...
    //If this ends up being a performance bottle neck, then I'll change it....
    
    for (int i = 0; i < system->system_clock->delta_time; ++i) {
        //If remaining cycles is 0 (DMA transfer just finished), reset it and exit
        if (system->memory->remaining_dma_cycles == 0) {
            system->memory->remaining_dma_cycles = 640;
            system->memory->dma_active = 0;
            break;
        }        

        //DMA transfer transfers from 0xXX00-0xXX9F to 0xFE00 to 0xFE9F
        //Writes happen on m-cycles, so we can only check this for modulo 4
        uint16_t remaining_cycles = system->memory->remaining_dma_cycles;
        if (remaining_cycles % 4 == 0) {
            uint16_t src_address = ((system->memory->dma_source << 8) | 0x9F) - remaining_cycles;
            uint16_t dest_address = 0xFE9F - remaining_cycles;

            uint8_t val = mem_read(system->memory, src_address, DMA_ACCESS);
            mem_write(system->memory, dest_address, val, DMA_ACCESS);
        }

        --system->memory->remaining_dma_cycles;
    }
}

//Requests STAT interrupt if met. Also updates LYC
void update_stat_register(EmulatorSystem* system) {
    //Okay I'm going to be SO honest because this is getting hard to keep track of the weird things..
    //I'm not sure if updating this should be HERE or should be LATER. It IS a hardware register, it needs
    //to be checked after each instruction like other hardware registers like the timing registers.... I dunno where else it'd go
    //I once again am unsure if this will be desynced by 1-2 m-cycles..

    //I did not include this in a loop because whether the interrupt is set or not
    //only matters when interrupts are serviced, which is after hardware udpates..
    //Also, lyc DOES get checked regularly, but since no memory changes happen here except
    //immediately after an instruction, it shouldn't change anything(?)

    //If you can't tell this is the point where all of the information is starting to get
    //so specific that keeping track of the timing quirks and individual little things is getting VERY difficult
    uint8_t current_stat_status = 0; //Start unset
    uint8_t* current_stat_address = &system->memory->io[0x41]; //Current stat value
    //When LY and LYC (0xFF44 and 0xFF45) are equal, it sets bit 2 of STAT
    uint8_t lyc = system->memory->io[0x44] == system->memory->io[0x45];
    PPU_Mode ppu_mode = system->memory->current_ppu_mode;

    if (lyc)
        *current_stat_address |= (1 << 2);

    //Updates PPU mode as well?
    //Since PPU updates this, this will probably just make sure that the STAT register
    //reflects the correct value, but the enum in the memory module will still be the
    //way it gets checked for certain access things

    //0s bottom 2 bits for STAT and merges with current PPU mode
    *current_stat_address = (*current_stat_address & 0xFC) | ppu_mode;

    //Check each bit for its condition. Only 1 needs to be true to trigger a STAT interrupt

    //Bit 3 - PPU mode 0
    if ((GET_BIT(*current_stat_address, 3)) && ppu_mode == PPU_MODE_0)
        current_stat_status = 1;

    //Bit 4 - PPU mode 1
    if ((GET_BIT(*current_stat_address, 4)) && ppu_mode == PPU_MODE_1)
        current_stat_status = 1;

    //Bit 5 - PPU mode 2
    if ((GET_BIT(*current_stat_address, 5)) && ppu_mode == PPU_MODE_2)
        current_stat_status = 1;

    //Bit 6 - LY = LYC
    if ((GET_BIT(*current_stat_address, 6)) && lyc)
        current_stat_status = 1;

    uint8_t prev_stat_status = system->memory->stat_interrupt_state;

    //This gets set on a rising edge, which is why we store the previous state
    if (prev_stat_status == 0 && current_stat_status == 1)
        requestInterrupt(INTERRUPT_LCD, system->cpu);

    //Update stat interrupt
    system->memory->stat_interrupt_state = current_stat_status;
}