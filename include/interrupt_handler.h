#ifndef INTERRUPT_HANDLER_H
#define INTERRUPT_HANDLER_H 

//I'm not sure exactly how the heirarchy should work out..
//But since everything basically sends interrupt requests to the CPU, I think that this makes sense...
//So everybody can request interrupts, and this can access the CPU to service them.
#include "cpu.h"

typedef enum {
    INTERRUPT_TIMER
} Interrupt;

void requestInterrupt(Interrupt, Memory*);

#endif