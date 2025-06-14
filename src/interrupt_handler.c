#include "interrupt_handler.h"

void requestInterrupt(Interrupt interrupt, Memory* mem) {
    if (interrupt == INTERRUPT_TIMER)
        mem->io[0x0F] |= 1 << 2; //IF is 0xFF0F, bit 2 is timer interrupt
}