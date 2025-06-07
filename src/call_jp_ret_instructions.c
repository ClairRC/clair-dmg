#include "instructions.h"

//Calls function at address
int call_imm16(CPU* cpu, Instruction* instruction){
    //Get values
    uint8_t lsb = mem_read(cpu->registers.pc++);
    uint8_t msb = mem_read(cpu->registers.pc++);
    uint16_t target_address = UNSIGNED_16(lsb, msb);

    //Save return address to stack
    mem_write(--cpu->registers.sp, GET_MSB(cpu->registers.pc));
    mem_write(--cpu->registers.sp, GET_LSB(cpu->registers.pc));

    //Set new PC
    cpu->registers.pc = target_address;

    //24 t-cycles
    return 24;
}

//Calls function at immediate 16-bit address if flag is set/cleared
int call_flag_imm16(CPU* cpu, Instruction* instruction) {
    //Get values
    uint8_t lsb = mem_read(cpu->registers.pc++);
    uint8_t msb = mem_read(cpu->registers.pc++);
    uint16_t target_address = UNSIGNED_16(lsb, msb);

    //Check condition
    //If flag is SET/CLEAR, do the jump, otherwise, don't do that
    if (flagIsSet(cpu, instruction->first_operand) == instruction->second_operand) {
        //Save return address to stack
        mem_write(--cpu->registers.sp, GET_MSB(cpu->registers.pc));
        mem_write(--cpu->registers.sp, GET_LSB(cpu->registers.pc));

        //Set the new pc
        cpu->registers.pc = target_address;

        //24 t-cycles if it jumps
        return 24;
    }
    else
        //12 t-cycles if no jump
        return 12;
}

//Calls function at pre-determined address
int rst_imm16(CPU* cpu, Instruction* instruction) {
    //Save return address to stack
    mem_write(--cpu->registers.sp, GET_MSB(cpu->registers.pc));
    mem_write(--cpu->registers.sp, GET_LSB(cpu->registers.pc));

    //Jump to address
    cpu->registers.pc = UNSIGNED_16(instruction->first_operand, 0x00);

    //16 t-cycles
    return 16;
}

//Jumps to immediate 16-bit address
int jp_imm16(CPU* cpu, Instruction* instruction) {
    //Get values
    uint8_t lsb = mem_read(cpu->registers.pc++);
    uint8_t msb = mem_read(cpu->registers.pc++);
    uint16_t target_address = UNSIGNED_16(lsb, msb);

    //Jump
    cpu->registers.pc = target_address;

    //16 t-cycles
    return 16;
}

//Jumps to immediate 16-bit address if flag is set/clear
int jp_flag_imm16(CPU* cpu, Instruction* instruction) {
    //Get values
    uint8_t lsb = mem_read(cpu->registers.pc++);
    uint8_t msb = mem_read(cpu->registers.pc++);
    uint16_t target_address = UNSIGNED_16(lsb, msb);

    //Check condition
    //If true, jump
    if (flagIsSet(cpu, instruction->first_operand) == instruction->second_operand) {
        cpu->registers.pc = target_address;
        //16 t-cycles when jump occurs
        return 16;
    }
    else 
        // t-cycles when no jump
        return 12;
}

//Jumps to address stored in Hl
int jp_hl(CPU* cpu, Instruction* instruction) {
    uint16_t address = getRegisterValue16(cpu, REG_HL);
    cpu->registers.pc = address;

    //4 t-cycles
    return 4;
}

//Jumps to relative address
int jr_imm8s(CPU* cpu, Instruction* instruction) {
    //Get offset
    int8_t offset = (int8_t)mem_read(cpu->registers.pc++);
    
    //Jump
    cpu->registers.pc += (int8_t)offset;

    //12 t-cycles
    return 12;
}

//Jumps to relative address if flag is set/cleared
int jr_flag_imm8s(CPU* cpu, Instruction* instruction) {
    //Get offset
    int8_t offset = (int8_t)mem_read(cpu->registers.pc++);

    //If flag condition is true jump
    if (flagIsSet(cpu, instruction->first_operand) == instruction->second_operand) {
        cpu->registers.pc += (int8_t) offset;

        //12 t-cycles if jump happens
        return 12;
    }
    else
        //8 t-cycles if no jump
        return 8;
}

//Returns from function call
int ret(CPU* cpu, Instruction* instruction) {
    //Gets return address off stack
    uint8_t lsb = mem_read(cpu->registers.sp++);
    uint8_t msb = mem_read(cpu->registers.sp++);

    //Jumps back
    cpu->registers.pc = UNSIGNED_16(lsb, msb);

    //16 t-cycles
    return 16;
}

//Returns from function call if flag is set/clear
int ret_flag(CPU* cpu, Instruction* instruction) {
    //Checks condition then returns if true
    if (flagIsSet(cpu, instruction->first_operand) == instruction->second_operand) {
        //Get address off stack
        uint8_t lsb = mem_read(cpu->registers.sp++);
        uint8_t msb = mem_read(cpu->registers.sp++);

        cpu->registers.pc = UNSIGNED_16(lsb, msb);

        //20 t-cycles if return
        return 20;
    }
    else 
        //8 t-cycles if no return
        return 8;
}

//Returns from interrupt handler
int reti(CPU* cpu, Instruction* instruction) {
    //Get return address off stack
    uint8_t lsb = mem_read(cpu->registers.sp++);
    uint8_t msb = mem_read(cpu->registers.sp++);

    //Jump to address
    cpu->registers.pc = UNSIGNED_16(lsb, msb);

    //Enables IME
    setFlag(cpu, IME);

    //16 t-cycles
    return 16;
}