#include "instructions.h"

//Increments value stored in 16-bit register
int inc_r16(CPU* cpu, Instruction* instruction) {
    //Get value
    uint16_t val = getRegisterValue16(cpu, instruction->first_operand);

    //Store result
    setRegisterValue(cpu, instruction->first_operand, val + 1);

    //8 t-cycles
    return 8;
}

//Increments value in 8-bit register
int inc_r8(CPU* cpu, Instruction* instruction) {
    //Get value
    uint8_t val = getRegisterValue8(cpu, instruction->first_operand);

    //Store result
    setRegisterValue(cpu, instruction->first_operand, val + 1);

    //Update flags
    clearFlag(cpu, SUB);
    updateFlag(cpu, ZERO, CHECK_ZERO_8(val + 1));
    updateFlag(cpu, HALFCARRY, (val & 0xF) == 0xF); //If lower nibble is 0xF, then adding one results in half carry

    //4 t-cycles
    return 4;
}

//Increments value at address stored in 16-bit register
int inc_r16mem(CPU* cpu, Instruction* instruction) {
    //Get value
    uint16_t src_address = getRegisterValue16(cpu, instruction->first_operand);
    uint8_t val = mem_read(src_address);

    //Store result
    mem_write(src_address, val + 1);

    //Update flags
    clearFlag(cpu, SUB);
    updateFlag(cpu, ZERO, CHECK_ZERO_8(val + 1));
    updateFlag(cpu, HALFCARRY, (val & 0xF) == 0xF);

    //12 t-cycles
    return 12;
}

//Decrements value in 16-bit register
int dec_r16(CPU* cpu, Instruction* instruction) {
    //Get value
    uint16_t val = getRegisterValue16(cpu, instruction->first_operand);

    //Store result
    setRegisterValue(cpu, instruction->first_operand, val - 1);

    //8 t-cycles
    return 8;
}

//Decrements value in 8-bit register
int dec_r8(CPU* cpu, Instruction* instruction) {
    //Get value
    uint8_t val = getRegisterValue8(cpu, instruction->first_operand);

    //Store result
    setRegisterValue(cpu, instruction->first_operand, val - 1);

    //Update flags
    setFlag(cpu, SUB);
    updateFlag(cpu, ZERO, CHECK_ZERO_8(val - 1));
    updateFlag(cpu, HALFCARRY, (val & 0xF) == 0x0); //If lower nibble equals 0, then decrement sets half carry

    //4 t-cycles
    return 4;
}

//Decrements value at address stored in 16-bit register
int dec_r16mem(CPU* cpu, Instruction* instruction) {
    //Get value
    uint16_t src_address = getRegisterValue16(cpu, instruction->first_operand);
    uint8_t val = mem_read(src_address);

    //Store result
    mem_write(src_address, val - 1);

    //Update flags
    setFlag(cpu, SUB);
    updateFlag(cpu, ZERO, CHECK_ZERO_8(val - 1));
    updateFlag(cpu, HALFCARRY, (val & 0xF) == 0x0);

    //12 t-cycles
    return 12;
}