#ifndef INTERRUPT_HANDLER_H
#define INTERRUPT_HANDLER_H 

//I'm not sure exactly how the heirarchy should work out..
//But since everything basically sends interrupt requests to the CPU, I think that this makes sense...
//So everybody can request interrupts, and this can access the CPU to service them.
#include "cpu.h"

typedef enum {
    INTERRUPT_VBLANK = 0,
    INTERRUPT_LCD = 1,
    INTERRUPT_TIMER = 2,
    INTERRUPT_SERIAL = 3,
    INTERRUPT_JOYPAD = 4
} Interrupt;

int serviceInterrupt(CPU*);
int interruptPending(Interrupt, CPU*);
int anyInterruptPending(CPU*);
void requestInterrupt(Interrupt, CPU*);
void clearInterrupt(Interrupt, CPU*);

#endif