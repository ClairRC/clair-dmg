#include "instructions.h" 

//Shifts bits right, replaces msb with lsb. Value from accumulator
int rrca(CPU* cpu, Instruction* instruction) {
    //Get values
    uint8_t val = getRegisterValue8(cpu, REG_A);
    int bit0 = val & 0x1; //Left with the least significant bit, which gets shifted out

    //Store result
    setRegisterValue(cpu, REG_A, (val >> 1) | (bit0 << 7));

    //Update flags 
    clearFlag(cpu, SUB);
    clearFlag(cpu, ZERO);
    clearFlag(cpu, HALFCARRY);
    updateFlag(cpu, CARRY, bit0); //If bit0 was 0, clear flag. If it was 1, set flag

    //0 extra t-cycles
    return 0;
}

//Shifts bits right, reaplaces msb with carry bit
int rra(CPU* cpu, Instruction* instruction) {
    //Get values
    uint8_t val = getRegisterValue8(cpu, REG_A);
    int carry = flagIsSet(cpu, CARRY);

    //Store result
    setRegisterValue(cpu, REG_A, (val >> 1) | (carry << 7));

    //Update flags
    clearFlag(cpu, SUB);
    clearFlag(cpu, ZERO);
    clearFlag(cpu, HALFCARRY);
    updateFlag(cpu, CARRY, val & 0x1);

    //0 extra t-cycles
    return 0;
}

//Shifts bit left, replaces lsb with previos msb
int rlca(CPU* cpu, Instruction* instruction) {
    //Get values
    uint8_t val = getRegisterValue8(cpu, REG_A);
    int bit7 = val & (1 << 7);
    
    //Store result
    setRegisterValue(cpu, REG_A, (val << 1) | (bit7 >> 7));

    //Update flags
    clearFlag(cpu, SUB);
    clearFlag(cpu, ZERO);
    clearFlag(cpu, HALFCARRY);
    updateFlag(cpu, CARRY, bit7 >> 7);

    //0 extra t-cycles
    return 0;
}

//Shifts bits left, replaces lsb with carry bit
int rla(CPU* cpu, Instruction* instruction) {
    //Get values
    uint8_t val = getRegisterValue8(cpu, REG_A);
    int bit7 = val & (1 << 7);
    int carry = flagIsSet(cpu, CARRY);

    //Store result
    setRegisterValue(cpu, REG_A, (val << 1) | carry);

    //Update flags
    clearFlag(cpu, SUB);
    clearFlag(cpu, ZERO);
    clearFlag(cpu, HALFCARRY);
    updateFlag(cpu, CARRY, bit7 >> 7);

    //0 extra t-cycles
    return 0;
}