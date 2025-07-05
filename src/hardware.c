#include "hardware.h"
#include "interrupt_handler.h"

//Updates DMA, which writes to memory
void update_dma_transfer(EmulatorSystem* system) {
    //If DMA is not active, which is usually true, return immediately
    if (!system->memory->state.dma_active)
        return;

    //Decrement remaining cycles, this allows for 160 to not be included and 0 to be included, which is accurate start/end timing
    --system->memory->state.remaining_dma_cycles;

    //DMA transfer transfers from 0xXX00-0xXX9F to 0xFE00 to 0xFE9F
    uint16_t remaining_cycles = system->memory->state.remaining_dma_cycles;
    
    //1 transfer is completed every 4 ticks (each m cycle)
    //Only do the write if we are below the "start" point

    if (remaining_cycles % 4 == 0) {
        //DMA works like echo RAM
        //So I am simulating this by just putting the offset into 0xDE range instead
        if (system->memory->state.dma_source >= 0xFE) {
            system->memory->state.dma_source -= 0xFE;
            system->memory->state.dma_source += 0xDE;
        }

        uint16_t src_address = ((uint16_t)(system->memory->state.dma_source << 8) | 0x00) + ((640 - remaining_cycles)/4) - 1; //Divide by 4 since each write happens every 4 ticks
        uint16_t dest_address = 0xFE00 + (640 - remaining_cycles)/4 - 1;

        uint8_t val = mem_read(system->memory, src_address, DMA_ACCESS);
        mem_write(system->memory, dest_address, val, DMA_ACCESS);
    }

    //If remaining cycles is 0 (DMA transfer just finished), reset it
    if (system->memory->state.remaining_dma_cycles == 0) {
        system->memory->state.remaining_dma_cycles = 640;
        system->memory->state.dma_active = 0;
    }
}
