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

        //If CPU is halted, simulate by not incrementing PC
        //TODO: Implement HALT bug
        if (cpu->state.isHalted)
            --cpu->registers.pc;

        //Fetch intsruction
        uint8_t opcode = mem_read(mem, cpu->registers.pc++, CPU_ACCESS);

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
        delta_time = 0; //Reset delta time...

        //Service any interrupts
        if (cpu->state.IME) {
            delta_time += serviceInterrupt(cpu);
            //Update other hardware state again...
            sync_hardware(system, delta_time);
        }

        //If EI was called, enable IME now...
        if (cpu->state.enableIME) {
            cpu->state.IME = 1;
            cpu->state.enableIME = 0;
        }
    }
}