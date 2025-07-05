#include "instructions.h"

//bitwise and between 8-bit register and 8-bit register, and stores the result in first register
int and_r8_r8(CPU* cpu, Instruction* instruction) {
    //Get values
    uint8_t firstOp = getRegisterValue8(cpu, instruction->first_operand);
    uint8_t secondOp = getRegisterValue8(cpu, instruction->second_operand);
    
    uint8_t result = firstOp & secondOp;

    //Store result
    setRegisterValue(cpu, instruction->first_operand, result);

    //Update flags
    clearFlag(cpu, SUB);
    updateFlag(cpu, ZERO, CHECK_ZERO_8(result));
    clearFlag(cpu, CARRY);
    setFlag(cpu, HALFCARRY);

    //0 extra t-cycles
    return 0;
}

//bitwise and between 8-bit register and value at addrss stored in 16-bit register, and stores the result in the 8-bit register
int and_r8_r16mem(CPU* cpu, Instruction* instruction) {
    //Get values
    uint16_t src_address = getRegisterValue16(cpu, instruction->second_operand);
    uint8_t firstOp = getRegisterValue8(cpu, instruction->first_operand);
    uint8_t secondOp = mem_read(cpu->memory, src_address, CPU_ACCESS);

    uint8_t result = firstOp & secondOp;

    //Store result
    setRegisterValue(cpu, instruction->first_operand, result);

    //Update flags
    clearFlag(cpu, SUB);
    updateFlag(cpu, ZERO, CHECK_ZERO_8(result));
    clearFlag(cpu, CARRY);
    setFlag(cpu, HALFCARRY);

    //0 extra t-cycles
    return 0;
}

//Bitwise and between 8-bit immediate and value in 8-bit register. Result is stored in 8-bit register
int and_r8_imm8(CPU* cpu, Instruction* instruction) {
    //Get values
    uint8_t firstOp = getRegisterValue8(cpu, instruction->first_operand);
    uint8_t secondOp = mem_read(cpu->memory, cpu->registers.pc++, CPU_ACCESS);

    uint8_t result = firstOp & secondOp;

    //Store value
    setRegisterValue(cpu, instruction->first_operand, result);

    //Update flags
    clearFlag(cpu, SUB);
    updateFlag(cpu, ZERO, CHECK_ZERO_8(result));
    clearFlag(cpu, CARRY);
    setFlag(cpu, HALFCARRY);

    //0 extra t-cycles
    return 0;
}

//Bitwise xor between two 8-bit registers. Results is stored in first register
int xor_r8_r8(CPU* cpu, Instruction* instruction) {
    //Get values
    uint8_t firstOp = getRegisterValue8(cpu, instruction->first_operand);
    uint8_t secondOp = getRegisterValue8(cpu, instruction->second_operand);

    uint8_t result = firstOp ^ secondOp;

    //Store result
    setRegisterValue(cpu, instruction->first_operand, result);

    //Update flags
    clearFlag(cpu, SUB);
    updateFlag(cpu, ZERO, CHECK_ZERO_8(result));
    clearFlag(cpu, CARRY);
    clearFlag(cpu, HALFCARRY);

    //0 extra t-cycles
    return 0;
}

//Bitwise xor between value in 8-bit regsiter and value at address stored in 16-bit register. Result is stored in 8-bit register
int xor_r8_r16mem(CPU* cpu, Instruction* instruction) {
    //Get values
    uint8_t firstOp = getRegisterValue8(cpu, instruction->first_operand);
    uint16_t src_address = getRegisterValue16(cpu, instruction->second_operand);
    uint8_t secondOp = mem_read(cpu->memory, src_address, CPU_ACCESS);

    uint8_t result = firstOp ^ secondOp;

    //Store result
    setRegisterValue(cpu, instruction->first_operand, result);

    //Update flags
    clearFlag(cpu, SUB);
    updateFlag(cpu, ZERO, CHECK_ZERO_8(result));
    clearFlag(cpu, CARRY);
    clearFlag(cpu, HALFCARRY);

    //0 extra t-cycles
    return 0;
}

//Bitwise xor between 8-bit immediate and value in 8-bit register. result is stored in register
int xor_r8_imm8(CPU* cpu, Instruction* instruction) {
    //Get values
    uint8_t firstOp = getRegisterValue8(cpu, instruction->first_operand);
    uint8_t secondOp = mem_read(cpu->memory, cpu->registers.pc++, CPU_ACCESS);

    uint8_t result = firstOp ^ secondOp;

    //Store result
    setRegisterValue(cpu, instruction->first_operand, result);

    //Update flags
    clearFlag(cpu, SUB);
    updateFlag(cpu, ZERO, CHECK_ZERO_8(result));
    clearFlag(cpu, CARRY);
    clearFlag(cpu, HALFCARRY);

    //0 extra t-cycles
    return 0;
}

//Bitwise or between 2 8-bit regsiters. result is stored in first register
int or_r8_r8(CPU* cpu, Instruction* instruction) {
    //Get values
    uint8_t firstOp = getRegisterValue8(cpu, instruction->first_operand);
    uint8_t secondOp = getRegisterValue8 (cpu, instruction->second_operand);

    uint8_t result = firstOp | secondOp;

    //Store result
    setRegisterValue(cpu, instruction->first_operand, result);

    //Update flags
    clearFlag(cpu, SUB);
    updateFlag(cpu, ZERO, CHECK_ZERO_8(result));
    clearFlag(cpu, CARRY);
    clearFlag(cpu, HALFCARRY);

    //0 extra t-cycles
    return 0;
}

//Bitwise or between 8-bit register and value at address stored in 16-bit register. Result is stroed in 8-bit register
int or_r8_r16mem(CPU* cpu, Instruction* instruction) {
    //Get values
    uint8_t firstOp = getRegisterValue8(cpu, instruction->first_operand);
    uint16_t src_address = getRegisterValue16(cpu, instruction->second_operand);
    uint8_t secondOp = mem_read(cpu->memory, src_address, CPU_ACCESS);

    uint8_t result = firstOp | secondOp;

    //Store result
    setRegisterValue(cpu, instruction->first_operand, result);

    //Update flags
    clearFlag(cpu, SUB);
    updateFlag(cpu, ZERO, CHECK_ZERO_8(result));
    clearFlag(cpu, CARRY);
    clearFlag(cpu, HALFCARRY);

    //0 extra t-cycles
    return 0;
}

//bitwise or between 8-bit immediate and 8-bit register. Result is stored in register
int or_r8_imm8(CPU* cpu, Instruction* instruction) {
    //Get values
    uint8_t firstOp = getRegisterValue8(cpu, instruction->first_operand);
    uint8_t secondOp = mem_read(cpu->memory, cpu->registers.pc++, CPU_ACCESS);

    uint8_t result = firstOp | secondOp;

    //Store result
    setRegisterValue(cpu, instruction->first_operand, result);

    //Update flags
    clearFlag(cpu, SUB);
    updateFlag(cpu, ZERO, CHECK_ZERO_8(result));
    clearFlag(cpu, CARRY);
    clearFlag(cpu, HALFCARRY);

    //0 extra t-cycles
    return 0;
}