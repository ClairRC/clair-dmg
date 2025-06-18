#include "fe_de_ex.h" 
#include "instructions.h"
#include "hardware.h"
#include "interrupt_handler.h"

//Begins instruction loop and handles all of that fun stuff...
int fe_de_ex(EmulatorSystem* system) {
    CPU* cpu = system->cpu;
    Memory* mem = system->memory;

    while (1) {
        uint16_t delta_time = 0;

        //If there are interrupts pending...
        if (anyInterruptPending(cpu)) {
            clearFlag(cpu, IS_HALTED); //Unhalt CPU if halted

            //If IME is enabled, service the interrupts...
            if(flagIsSet(cpu, IME)) {
                delta_time += serviceInterrupt(cpu);
                //Update other hardware state again...
                sync_hardware(system, delta_time);
                delta_time = 0; //Reset delta time
            }
        }

        //If EI was called, enable IME now...
        if (cpu->state.enableIME) {
            cpu->state.IME = 1;
            cpu->state.enableIME = 0;
        }

        //Fetch intsruction
        uint8_t opcode = mem_read(mem, cpu->registers.pc++, CPU_ACCESS);

        //If we are halted, the CPU does nothing here, which effectively is a NOP and PC stays the same
        //So to simulate this I will just manually change the opcode to be a NOP
        //and move the PC back down
        if (flagIsSet(cpu, IS_HALTED)) {
            opcode = 0x00;
            --cpu->registers.pc;
        }

        //If HALT bug is active, it'll reread the instruction, so decrement sp
        //PC literally just fails to increment with halt bug, so this is accurate behavior
        if (flagIsSet(cpu, HALT_BUG)) {
            --cpu->registers.pc;
            clearFlag(cpu, HALT_BUG);
        }

        //Decode instruction
        Instruction* instr;
        if (opcode != 0xCB) {
            //If opcode is not CB, then get the instruction from normal table..
            instr = &main_instructions[opcode];
        }
        else {
            //Otherwise, get the CB instruction
            opcode = mem_read(mem, cpu->registers.pc++, CPU_ACCESS);
            instr = &cb_instructions[opcode];
        }

        //Execute instruction
        delta_time += instr->funct_ptr(cpu, instr);

        //Update other hardware state...
        sync_hardware(system, delta_time);
        delta_time = 0;
    }

    return 0;
}