#include "interrupt_handler.h"

//Services interrupt
int serviceInterrupt(CPU* cpu) {
    //This process basically just calls the interrupt itself. It takes 20 t-cycles
    
    //If there is no interrupt pending in the IF flag, do nothing. No t-cycles consumed
    uint8_t IF = cpu->memory->io[0x0F];
    if (IF == 0x0)
        return 0;

    //Save return address to stack
    uint8_t pc_lsb = (uint8_t)(cpu->registers.pc);
    uint8_t pc_msb = (uint8_t)(cpu->registers.pc >> 8);

    mem_write(cpu->memory, --cpu->registers.sp, pc_msb, CPU_ACCESS);
    mem_write(cpu->memory, --cpu->registers.sp, pc_lsb, CPU_ACCESS);

    uint16_t target_address = 0x0;

    //Gets highest priority interrupt address and clears pending interrupt
    if (interruptPending(INTERRUPT_VBLANK, cpu)) {
        target_address = 0x40;
        clearInterrupt(INTERRUPT_VBLANK, cpu);
    }

    else if (interruptPending(INTERRUPT_LCD, cpu)) {
        target_address = 0x48;
        clearInterrupt(INTERRUPT_LCD, cpu);
    }

    else if (interruptPending(INTERRUPT_TIMER, cpu)) {
        target_address = 0x50;
        clearInterrupt(INTERRUPT_TIMER, cpu);
    }

    else if (interruptPending(INTERRUPT_SERIAL, cpu)) {
        target_address = 0x58;
        clearInterrupt(INTERRUPT_SERIAL, cpu);
    }

    else if (interruptPending(INTERRUPT_JOYPAD, cpu)) {
        target_address = 0x60;
        clearInterrupt(INTERRUPT_JOYPAD, cpu);
    }

    clearFlag(cpu, IME); //Reset IME
    cpu->registers.pc = target_address; //Jumps to address

    //20 t-cycles for this whole process
    return 20;
}

//Checks if interrupt can be serviced
int interruptPending(Interrupt interrupt, CPU* cpu) {
    uint8_t IE = cpu->memory->hram[0x7F];
    uint8_t IF = cpu->memory->io[0x0F];

    //Returns 1 if bit is both enabled in IE and waiting in IF
    return ((IE >> interrupt) & 0x01) && ((IF >> interrupt) & 0x01);    
}

//Sets interrupt flag
void requestInterrupt(Interrupt interrupt, CPU* cpu) {
    uint8_t* IF = &cpu->memory->io[0x0F];

    *IF |= (1 << interrupt);
}

//Clears interrupt
void clearInterrupt(Interrupt interrupt, CPU* cpu) {
    uint8_t* IF = &cpu->memory->io[0x0F];

    *IF &= ~(1 << interrupt);
}