#include "instructions.h"

//Calls function at address
int call_imm16(CPU* cpu, Instruction* instruction){
    //Get values
    uint8_t lsb = mem_read(cpu->memory, cpu->registers.pc++, CPU_ACCESS);
    uint8_t msb = mem_read(cpu->memory, cpu->registers.pc++, CPU_ACCESS);
    uint16_t target_address = UNSIGNED_16(lsb, msb);

    //Save return address to stack
    if (!mem_write(cpu->memory, cpu->registers.sp - 1, GET_MSB(cpu->registers.pc), CPU_ACCESS)) { --cpu->registers.sp; }
    if (!mem_write(cpu->memory, cpu->registers.sp - 1, GET_LSB(cpu->registers.pc), CPU_ACCESS)) { --cpu->registers.sp; }

    //Set new PC
    cpu->registers.pc = target_address;

    //0 extra t-cycles
    return 0;
}

//Calls function at immediate 16-bit address if flag is set/cleared
int call_flag_imm16(CPU* cpu, Instruction* instruction) {
    //Get values
    uint8_t lsb = mem_read(cpu->memory, cpu->registers.pc++, CPU_ACCESS);
    uint8_t msb = mem_read(cpu->memory, cpu->registers.pc++, CPU_ACCESS);
    uint16_t target_address = UNSIGNED_16(lsb, msb);

    //Check condition
    //If flag is SET/CLEAR, do the jump, otherwise, don't do that
    if (flagIsSet(cpu, instruction->first_operand) == instruction->second_operand) {
        //Save return address to stack
        if (!mem_write(cpu->memory, cpu->registers.sp - 1, GET_MSB(cpu->registers.pc), CPU_ACCESS)) { --cpu->registers.sp; }
        if (!mem_write(cpu->memory, cpu->registers.sp - 1, GET_LSB(cpu->registers.pc), CPU_ACCESS)) { --cpu->registers.sp; }

        //Set the new pc
        cpu->registers.pc = target_address;

        //12 extra t-cycles if it jumps
        return 12;
    }
    else
        //0 extra t-cycles if no jump
        return 0;
}

//Calls function at pre-determined address
int rst_imm16(CPU* cpu, Instruction* instruction) {
    //Save return address to stack
    if (!mem_write(cpu->memory, cpu->registers.sp - 1, GET_MSB(cpu->registers.pc), CPU_ACCESS)) { --cpu->registers.sp; }
    if (!mem_write(cpu->memory, cpu->registers.sp - 1, GET_LSB(cpu->registers.pc), CPU_ACCESS)) { --cpu->registers.sp; }

    //Jump to address
    cpu->registers.pc = UNSIGNED_16(instruction->first_operand, 0x00);

    //0 extra t-cycles
    return 0;
}

//Jumps to immediate 16-bit address
int jp_imm16(CPU* cpu, Instruction* instruction) {
    //Get values
    uint8_t lsb = mem_read(cpu->memory, cpu->registers.pc++, CPU_ACCESS);
    uint8_t msb = mem_read(cpu->memory, cpu->registers.pc++, CPU_ACCESS);
    uint16_t target_address = UNSIGNED_16(lsb, msb);

    //Jump
    cpu->registers.pc = target_address;

    //0 extra t-cycles
    return 0;
}

//Jumps to immediate 16-bit address if flag is set/clear
int jp_flag_imm16(CPU* cpu, Instruction* instruction) {
    //Get values
    uint8_t lsb = mem_read(cpu->memory, cpu->registers.pc++, CPU_ACCESS);
    uint8_t msb = mem_read(cpu->memory, cpu->registers.pc++, CPU_ACCESS);
    uint16_t target_address = UNSIGNED_16(lsb, msb);

    //Check condition
    //If true, jump
    if (flagIsSet(cpu, instruction->first_operand) == instruction->second_operand) {
        cpu->registers.pc = target_address;
        //4 extra t-cycles when jump occurs
        return 4;
    }
    else 
        //0 extra t-cycles if no jump
        return 0;
}

//Jumps to address stored in Hl
int jp_hl(CPU* cpu, Instruction* instruction) {
    uint16_t address = getRegisterValue16(cpu, REG_HL);
    cpu->registers.pc = address;

    //0 extra t-cycles
    return 0;
}

//Jumps to relative address
int jr_imm8s(CPU* cpu, Instruction* instruction) {
    //Get offset
    int8_t offset = (int8_t)mem_read(cpu->memory, cpu->registers.pc++, CPU_ACCESS);
    
    //Jump
    cpu->registers.pc += (int8_t)offset;

    //0 extra t-cycles
    return 0;
}

//Jumps to relative address if flag is set/cleared
int jr_flag_imm8s(CPU* cpu, Instruction* instruction) {
    //Get offset
    int8_t offset = (int8_t)mem_read(cpu->memory, cpu->registers.pc++, CPU_ACCESS);

    //If flag condition is true jump
    if (flagIsSet(cpu, instruction->first_operand) == instruction->second_operand) {
        cpu->registers.pc += (int8_t) offset;

        //4 t-cycles if jump happens
        return 4;
    }
    else
        //0 extra t-cycles if no jump
        return 0;
}

//Returns from function call
int ret(CPU* cpu, Instruction* instruction) {
    //Gets return address off stack
    uint8_t lsb = mem_read(cpu->memory, cpu->registers.sp++, CPU_ACCESS);
    uint8_t msb = mem_read(cpu->memory, cpu->registers.sp++, CPU_ACCESS);

    //Jumps back
    cpu->registers.pc = UNSIGNED_16(lsb, msb);

    //0 extra t-cycles
    return 0;
}

//Returns from function call if flag is set/clear
int ret_flag(CPU* cpu, Instruction* instruction) {
    //Checks condition then returns if true
    if (flagIsSet(cpu, instruction->first_operand) == instruction->second_operand) {
        //Get address off stack
        uint8_t lsb = mem_read(cpu->memory, cpu->registers.sp++, CPU_ACCESS);
        uint8_t msb = mem_read(cpu->memory, cpu->registers.sp++, CPU_ACCESS);

        cpu->registers.pc = UNSIGNED_16(lsb, msb);

        //12 extra t-cycles if return
        return 12;
    }
    else 
        //0 extra t-cycles if no jump
        return 0;
}

//Returns from interrupt handler
int reti(CPU* cpu, Instruction* instruction) {
    //Get return address off stack
    uint8_t lsb = mem_read(cpu->memory, cpu->registers.sp++, CPU_ACCESS);
    uint8_t msb = mem_read(cpu->memory, cpu->registers.sp++, CPU_ACCESS);

    //Jump to address
    cpu->registers.pc = UNSIGNED_16(lsb, msb);

    //Enables IME
    setFlag(cpu, IME);

    //0 extra t-cycles
    return 0;
}