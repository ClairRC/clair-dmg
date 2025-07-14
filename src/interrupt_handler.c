#include "interrupt_handler.h"
#include "hardware_def.h"
#include <stdio.h>

//Services interrupt
void serviceInterrupt(CPU* cpu) {
    //This process basically just calls the interrupt itself. It takes 20 t-cycles

    //Save return address to stack
    uint8_t pc_lsb = (uint8_t)(cpu->registers.pc);
    uint8_t pc_msb = (uint8_t)(cpu->registers.pc >> 8);

    if (!mem_write(cpu->bus, cpu->registers.sp - 1, pc_msb, CPU_ACCESS)) { --cpu->registers.sp; }
    if (!mem_write(cpu->bus, cpu->registers.sp - 1, pc_lsb, CPU_ACCESS)) { --cpu->registers.sp; }

    //Default value in case SOMEHOW this function is called when an interrupt is not actually pending, so no jump will happen
    uint16_t target_address = 0x0;

    //Gets highest priority interrupt address and clears pending interrupt
    if (interruptPending(INTERRUPT_VBLANK, cpu->bus->memory)) {
        target_address = 0x40;
        clearInterrupt(INTERRUPT_VBLANK, cpu->bus->memory);
    }

    else if (interruptPending(INTERRUPT_LCD, cpu->bus->memory)) {
        target_address = 0x48;
        clearInterrupt(INTERRUPT_LCD, cpu->bus->memory);
    }

    else if (interruptPending(INTERRUPT_TIMER, cpu->bus->memory)) {
        target_address = 0x50;
        clearInterrupt(INTERRUPT_TIMER, cpu->bus->memory);
    }

    else if (interruptPending(INTERRUPT_SERIAL, cpu->bus->memory)) {
        target_address = 0x58;
        clearInterrupt(INTERRUPT_SERIAL, cpu->bus->memory);
    }

    else if (interruptPending(INTERRUPT_JOYPAD, cpu->bus->memory)) {
        target_address = 0x60;
        clearInterrupt(INTERRUPT_JOYPAD, cpu->bus->memory);
    }

    clearFlag(cpu, IME); //Reset IME
    cpu->registers.pc = target_address; //Jumps to address
}

//Checks if specific interrupt can be serviced
int interruptPending(Interrupt interrupt, Memory* mem) {
    uint8_t IE = mem->hram[0x7F];
    uint8_t IF = mem->io[0x0F];

    return ((IE >> interrupt) & 0x01) && ((IF >> interrupt) & 0x01);    
}

//Checks if there are ANY interrupts that can be serviced
int anyInterruptPending(Memory* mem) {
    //If there are interrupts that are waiting AND can be serviced, set flag
    int interruptsWaiting = 0;
    for (int i = 0; i <= 4; ++i) {
        if (interruptPending(i, mem) == 1) 
            interruptsWaiting = 1;
    }

    return interruptsWaiting;
}

//Sets interrupt flag
void requestInterrupt(Interrupt interrupt, Memory* mem) {
    uint8_t* IF = &mem->io[0x0F];
    
    *IF |= (1 << interrupt);
}

//Clears interrupt
void clearInterrupt(Interrupt interrupt, Memory* mem) {
    uint8_t* IF = &mem->io[0x0F];

    *IF &= ~(1 << interrupt);
}