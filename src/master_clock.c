#include <stdlib.h>

#include "master_clock.h"
#include "interrupt_handler.h"
#include "hardware_def.h"
#include "ppu.h"

//Inline functions
#define GET_BIT(num, bit) ((num) >> (bit)) & 0x1 //Gets value of specific bit (starting at 0)

//PPU Frame definitions
#define VISIBLE_SCANLINES 144
#define SCANLINE_END 456
#define MODE_2_END 80
#define MODE_3_END 240
#define MODE_0_END 0
#define VBLANK_BEGIN (SCANLINE_END * VISIBLE_SCANLINES)
#define MODE_1_END 70224

#define MODE_2_TIME(frame_time) (frame_time) % SCANLINE_END //Current time into mode 2
#define MODE_3_TIME(frame_time) ((frame_time) % SCANLINE_END) - 80 //Current time into mode 3

//Useful bitwise definitions
#define TAC_CLOCK_SELECT 0x3 //Bottom 2 bits select TAC clock value
#define TAC_ENABLE 0x4 //Bit 2 is TAC enable

//Iniital values for system clock
MasterClock* master_clock_init() {
	MasterClock* sys_clock = (MasterClock*)malloc(sizeof(MasterClock));

    if (sys_clock == NULL)
        return NULL;

    sys_clock->system_time = 0; //System timer (~4MHz)
    sys_clock->frame_time = 0; //Elapsed frame time

    sys_clock->prev_tac_bit = 0; //Stores what the last Timer Control bit was for updating TIMA
    sys_clock->prev_tma_val = 0; //Stores previous M-Cycle's TMA value for TIMA overflow

    sys_clock->tima_overflow = 0; //Flag to check whether TIMA overflowed
    sys_clock->tima_overflow_buffer = 0; //Flag to check if TIMA overflowed 2 cycles ago for TIMA updates

    return sys_clock;
}

//Updates timer registers
void update_timing_registers(MasterClock* clock, Memory* mem) {
    uint16_t start_time = clock->system_time;

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
    uint8_t prev_tac_bit = clock->prev_tac_bit; //Previous value of TAC

    //If TIMA overflowed on the previous cycle, request the interrupt now
    if (clock->tima_overflow) {
        clock->tima_overflow = 0;
        //If TIMA gets written to, the overflow gets ignored
        if (mem->TIMA_LOCATION == 0) {
            clock->tima_overflow_buffer = 1;
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
            clock->tima_overflow = 1;
        }
    }

    //Update previous TAC bit value
    clock->prev_tac_bit = tac_bit;
}

//Updates DMA, which writes to memory
void update_dma_transfer(Memory* mem) {
    if (!mem->state.dma_active)
        return;

    //Decrement remaining cycles, this allows for 160 to not be included and 0 to be included, which is accurate start/end timing
    --mem->state.remaining_dma_cycles;

    //DMA transfer transfers from 0xXX00-0xXX9F to 0xFE00 to 0xFE9F
    uint16_t remaining_cycles = mem->state.remaining_dma_cycles;

    //1 transfer is completed every 4 ticks (each m cycle)
    //Only do the write if we are below the "start" point
    if (remaining_cycles % 4 == 0) {
        //DMA works like echo RAM
        //So I am simulating this by just putting the offset into 0xDE range instead
        if (mem->state.dma_source >= 0xFE) {
            mem->state.dma_source -= 0xFE;
            mem->state.dma_source += 0xDE;
        }

        uint16_t src_address = ((uint16_t)(mem->state.dma_source << 8) | 0x00) + ((640 - remaining_cycles) / 4) - 1; //Divide by 4 since each write happens every 4 ticks
        uint16_t dest_address = 0xFE00 + (640 - remaining_cycles) / 4 - 1;

        uint8_t val = mem_read(mem, src_address, DMA_ACCESS);
        mem_write(mem, dest_address, val, DMA_ACCESS);
    }

    //If remaining cycles is 0 (DMA transfer just finished), reset it
    if (mem->state.remaining_dma_cycles == 0) {
        mem->state.remaining_dma_cycles = 640;
        mem->state.dma_active = 0;
    }
}

//Updates PPU timing
void update_ppu(MasterClock* clock, PPU* ppu, uint32_t frame_time) {
    //Update PPU state
    update_ppu_state(clock, ppu, frame_time);

    //Update stat register and request interrupt if necessary
    update_stat(ppu, frame_time);

    //Depending on state, do different things
    switch (ppu->memory->state.current_ppu_mode) {
        case PPU_MODE_2:
            ppu_oam_scan(ppu, MODE_2_TIME(frame_time));
            break;

        case PPU_MODE_3:
            ppu_write_lcd(ppu, MODE_3_TIME(frame_time));
            break;
    }
}

//Updates PPU state
void update_ppu_state(MasterClock* clock, PPU* ppu, uint32_t frame_time) {
    //If LCDC.7 is 1, set PPU on else set it off
    if (ppu->memory->LCDC_LOCATION & (1 << 7)) {
        ppu->state.lcd_on = 1;
        ppu->memory->LY_LOCATION = frame_time / SCANLINE_END; //Update LY
    }
    else {
        ppu->state.lcd_on = 0;
        ppu->memory->state.current_ppu_mode = PPU_MODE_OFF;
        ppu->memory->LY_LOCATION = 0; //LY is 0 if LCD is off
        clock->frame_time = 4; //Scanline starts at 4 when it turns back on

        return; //No updates are done if PPU is off
    }

    //If vblank ends and a new frame starts...
    if (frame_time == MODE_1_END) {
        clock->frame_time = 0; //Reset frame timer
        switch_mode_1_2(ppu);
    }

    //Only update the state if we are not in vblank

    uint16_t ly = ppu->memory->LY_LOCATION;
    //At the end of mode 2
    if (frame_time % SCANLINE_END == MODE_2_END && ppu->memory->state.current_ppu_mode != PPU_MODE_1)
        switch_mode_2_3(ppu);

    //At the end of mode 3
    if (frame_time % SCANLINE_END == MODE_3_END && ppu->memory->state.current_ppu_mode != PPU_MODE_1)
        switch_mode_3_0(ppu);

    //At end of mode 0
    if (frame_time % SCANLINE_END == MODE_0_END && ppu->memory->state.current_ppu_mode != PPU_MODE_1) {
        //If vblank is beginning, switch to vblank
        if (frame_time == VBLANK_BEGIN)
            switch_mode_0_1(ppu);

        //Otherwise, switch back to OAM scan
        else
            switch_mode_0_2(ppu);
    }
}

void update_stat(PPU* ppu, uint32_t frame_time) {
    //LYC is if LYC register and LY are equal
    uint8_t lyc = ppu->memory->LY_LOCATION == ppu->memory->LYC_LOCATION;
    uint8_t current_stat = 0; //Current interrupt status is 0 by default

    uint8_t* lcd_stat = &ppu->memory->STAT_LOCATION; //Pointer to stat register

    //Current PPU mode
    PPU_Mode current_mode = ppu->memory->state.current_ppu_mode;

    //Update PPU mode in stat register
    //Zeros bottom 2 bits and merges with PPU mode to keep it updated
    *lcd_stat = (*lcd_stat & 0xFC) | (current_mode & 0x03);

    //If LCY is true, it sets STAT bit 2
    //Otherwise clear it
    if (lyc)
        *lcd_stat |= (1 << 2);
    else
        *lcd_stat &= ~(1 << 2);

    //Trigger interrupt under these conditions..
    if (GET_BIT(*lcd_stat, 3) && (frame_time % SCANLINE_END) == MODE_3_END)
        current_stat = 1;
    if (GET_BIT(*lcd_stat, 4) && frame_time == VBLANK_BEGIN)
        current_stat = 1;
    if (GET_BIT(*lcd_stat, 5) && (frame_time % SCANLINE_END == 0))
        current_stat = 1;
    if (GET_BIT(*lcd_stat, 6) && GET_BIT(*lcd_stat, 2))
        current_stat = 1;

    //STAT interrupt triggers on rising edge..
    if (ppu->state.prev_stat == 0 && current_stat == 1) {
        requestInterrupt(INTERRUPT_LCD, ppu->memory);
    }

    //Update stat
    ppu->state.prev_stat = current_stat;
}