#include "fe_de_ex.h" 
#include "interrupt_handler.h"
#include "master_clock.h"

//Begins instruction loop and handles all of that fun stuff...
int fe_de_ex(EmulatorSystem* system) {
    CPU* cpu = system->cpu;
    MemoryBus* bus = system->bus;

    while (system->system_state->running) {
        check_interrupt(system); //Handles interrupts if there are any

        //If EI was called, enable IME now...
        if (cpu->state.enableIME) {
            cpu->state.IME = 1;
            cpu->state.enableIME = 0;
        }

        //Handle instruction
        //This doesn't happen if CPU is HALTed
        if (!flagIsSet(cpu, IS_HALTED)) {
            //Fetch instruction
            uint8_t opcode = fetch_instruction_opcode(system);

            //Decode instruction
            Instruction* instr = decode_instruction(system, opcode);

            //Execute instruction
            execute_instruction(system, instr);
        }
        //If CPU is HALTED, 4 cycles pass instead
        else {
            tick_hardware(system, 4);
        }
    }

    //When emulator closes, save data
    save_save_data(system->memory);

    return 0;
}

void check_interrupt(EmulatorSystem* system) {
    //If there are interrupts pending...
    if (anyInterruptPending(system->cpu->bus->memory)) {
        clearFlag(system->cpu, IS_HALTED); //Unhalt CPU if halted

        //If IME is enabled, service the interrupts...
        if (flagIsSet(system->cpu, IME)) {
            //Update hardware before interrupt occurs...
            //Interrupt always takes 20 t-cycles
            tick_hardware(system, 20);
            serviceInterrupt(system->cpu);
        }
    }
}

//Fetches instruction and handles HALT bug
uint8_t fetch_instruction_opcode(EmulatorSystem* system) {
    CPU* cpu = system->cpu;
    MemoryBus* bus = system->bus;

    uint8_t opcode = mem_read(bus, cpu->registers.pc++, CPU_ACCESS);

    //HALT bug will cause the PC to not increment for 1 instruction, but it will still continue execution.
    if (flagIsSet(cpu, HALT_BUG)) {
        --cpu->registers.pc;
        clearFlag(cpu, HALT_BUG);
    }

    return opcode;
}

//Decodes instruction and returns the correct pointer
Instruction* decode_instruction(EmulatorSystem* system, uint8_t opcode) {
    Instruction* instr;
    if (opcode != 0xCB) {
        //If opcode is not CB, then get the instruction from normal table..
        instr = &main_instructions[opcode];
    }
    else {
        //Otherwise, get the CB instruction
        opcode = mem_read(system->bus, system->cpu->registers.pc++, CPU_ACCESS);
        instr = &cb_instructions[opcode];
    }

    return instr;
}

//Executes instruction and ticks hardware
void execute_instruction(EmulatorSystem* system, Instruction* instr) {
    //Tick hardware before instruction executes to simulate it happening simultaneously
    uint8_t instr_time = instr->min_num_cycles;
    tick_hardware(system, instr_time);

    //Execute instruction and store extra cycles
    uint8_t extra_instr_time = instr->funct_ptr(system->cpu, instr);

    //If there are extra ticks (ie, a conditional jump occured), tick the remaining amount
    if (extra_instr_time != 0) {
        tick_hardware(system, extra_instr_time);
    }
}
