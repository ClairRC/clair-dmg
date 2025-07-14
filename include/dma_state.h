#ifndef DMA_STATE_H
#define DMA_STATE_H

#include <stdint.h>

typedef struct {
    uint8_t active; //DMA active flag
    uint16_t remaining_cycles; //Remaining cycles for the DMA transfer
    uint8_t source; //Source address for DMA transfer
} GlobalDMAState;

#endif