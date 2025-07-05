#include "instructions.h" 

//Helper functions depending on if register is 8-bit or HL
//I did these CB intsructions last, but I'm realizing now I could have done something
//like this EVERYWHERE else and probably cut down on a LOT of different functions I wrote...
//Oh well, lesson learned! Too late now!
uint8_t getVal(CPU* cpu, Registers reg) {
    uint8_t val;

    if (reg == REG_HL)
        val = mem_read(cpu->memory, getRegisterValue16(cpu, reg), CPU_ACCESS);
    else
        val = getRegisterValue8(cpu, reg);

    return val;
}

void setVal(CPU* cpu, Registers reg, uint8_t result) {
    if (reg == REG_HL)
        mem_write(cpu->memory, getRegisterValue16(cpu, reg), result, CPU_ACCESS);
    else
        setRegisterValue(cpu, reg, result);
}

//Rotate register value left circular
int rlc_r(CPU* cpu, Instruction* instruction) {
    //Get values
    uint8_t val = getVal(cpu, instruction->first_operand);
    int bit7 = val & (1 << 7);
    uint8_t result = (val << 1) | (bit7 >> 7);
    
    //Store result
    setVal(cpu, instruction->first_operand, result);

    //Update flags
    clearFlag(cpu, SUB);
    updateFlag(cpu, ZERO, CHECK_ZERO_8(result));
    clearFlag(cpu, HALFCARRY);
    updateFlag(cpu, CARRY, bit7 >> 7);

    //0 extra t-cycles
    return 0;
}

//Rotates register value right circular
int rrc_r(CPU* cpu, Instruction* instruction) {
    //Get values
    uint8_t val = getVal(cpu, instruction->first_operand);
    int bit0 = val & 0x1; //Left with the least significant bit, which gets shifted out
    uint8_t result = (val >> 1) | (bit0 << 7);

    //Store result
    setVal(cpu, instruction->first_operand, result);

    //Update flags 
    clearFlag(cpu, SUB);
    updateFlag(cpu, ZERO, CHECK_ZERO_8(result));
    clearFlag(cpu, HALFCARRY);
    updateFlag(cpu, CARRY, bit0); //If bit0 was 0, clear flag. If it was 1, set flag

    //0 extra t-cycles
    return 0;
}

//Rotates register value left
int rl_r(CPU* cpu, Instruction* instruction) {
    //Get values
    uint8_t val = getVal(cpu, instruction->first_operand);
    int bit7 = val & (1 << 7);
    int carry = flagIsSet(cpu, CARRY);
    uint8_t result = (val << 1) | carry;

    //Store result
    setVal(cpu, instruction->first_operand, result);

    //Update flags
    clearFlag(cpu, SUB);
    updateFlag(cpu, ZERO, CHECK_ZERO_8(result));
    clearFlag(cpu, HALFCARRY);
    updateFlag(cpu, CARRY, bit7 >> 7);

    //0 extra t-cycles
    return 0;
}

//Rotates register value right
int rr_r(CPU* cpu, Instruction* instruction) {
    //Get values
    uint8_t val = getVal(cpu, instruction->first_operand);
    int carry = flagIsSet(cpu, CARRY);
    uint8_t result = (val >> 1) | (carry << 7);
    
    //Store result
    setVal(cpu, instruction->first_operand, result);

    //Update flags
    clearFlag(cpu, SUB);
    updateFlag(cpu, ZERO, CHECK_ZERO_8(result));
    clearFlag(cpu, HALFCARRY);
    updateFlag(cpu, CARRY, val & 0x1);

    //0 extra t-cycles
    return 0;
}

//Shift register left arithmetic
int sla_r(CPU* cpu, Instruction* instruction) {
    //Get value
    uint8_t val = getVal(cpu, instruction->first_operand);
    int bit7 = val >> 7;
    uint8_t result = val << 1;

    //Store result
    setVal(cpu, instruction->first_operand, result);

    //Update flags
    clearFlag(cpu, SUB);
    updateFlag(cpu, ZERO, CHECK_ZERO_8(result));
    clearFlag(cpu, HALFCARRY);
    updateFlag(cpu, CARRY, bit7);

    //0 extra t-cycles
    return 0;
}

//Shift register right arithmetic
int sra_r(CPU* cpu, Instruction* instruction) {
    //Get Values
    uint8_t val = getVal(cpu, instruction->first_operand);
    int bit7 = val & (1 << 7);
    int bit0 = val & 0x1;
    uint8_t result = (val >> 1) | bit7; //Preserves most significant bit

    //Store result
    setVal(cpu, instruction->first_operand, result);

    //Update flags
    clearFlag(cpu, SUB);
    updateFlag(cpu, ZERO, CHECK_ZERO_8(result));
    clearFlag(cpu, HALFCARRY);
    updateFlag(cpu, CARRY, bit0);

    //0 extra t-cycles
    return 0;
}

//Swaps upper and lower nibbles of register
int swap_r(CPU* cpu, Instruction* instruction) {
    //Get values
    uint8_t val = getVal(cpu, instruction->first_operand);
    uint8_t result = (val << 4) | (val >> 4);

    //Store result
    setVal(cpu, instruction->first_operand, result);

    //Update flags
    clearFlag(cpu, SUB);
    clearFlag(cpu, HALFCARRY);
    clearFlag(cpu, CARRY);
    updateFlag(cpu, ZERO, CHECK_ZERO_8(result));

    //0 extra t-cycles
    return 0;
}

//Shift register right logical
int srl_r(CPU* cpu, Instruction* instruction) {
    //Get values
    uint8_t val = getVal(cpu, instruction->first_operand);
    int bit0 = val & 0x1;
    uint8_t result = val >> 1;

    //Store result
    setVal(cpu, instruction->first_operand, result);

    //Update flags
    clearFlag(cpu, SUB);
    updateFlag(cpu, ZERO, CHECK_ZERO_8(result));
    clearFlag(cpu, HALFCARRY);
    updateFlag(cpu, CARRY, bit0);

    //0 extra t-cycles
    return 0;
}

//Checks if specified bit is clear or set and updates flags
int bit_b_r(CPU* cpu, Instruction* instruction) {
    //Get values 
    uint8_t val = getVal(cpu, instruction->second_operand);
    int result = (val >> instruction->first_operand) & 0x1;

    //Update flags
    setFlag(cpu, HALFCARRY);
    clearFlag(cpu, SUB);
    updateFlag(cpu, ZERO, CHECK_ZERO_8(result));

    //0 extra t-cycles
    return 0;
}

//Resets bit at specified location
int res_b_r(CPU* cpu, Instruction* instruction) {
    //Get values
    uint8_t val = getVal(cpu, instruction->second_operand);
    uint8_t result = val & ~(1 << instruction->first_operand);

    //Store result
    setVal(cpu, instruction->second_operand, result);

    //0 extra t-cycles
    return 0;
}

//Sets bit at specified location
int set_b_r(CPU* cpu, Instruction* instruction) {
    //Get values
    uint8_t val = getVal(cpu, instruction->second_operand);
    uint8_t result = val | (1 << instruction->first_operand);

    //Store result
    setVal(cpu, instruction->second_operand, result);

    //0 extra t-cycles
    return 0;
}
