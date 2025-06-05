#include "cpu.h"
#include "instructions.h"

//Adds values from 2 8-bit regsiters and stores it into the first one
int add_r8_r8(CPU* cpu, Instruction* instruction) {
    //Get values
    uint8_t dest_val = getRegisterValue8(cpu, instruction->first_operand);
    uint8_t src_val = getRegisterValue8(cpu, instruction->second_operand);
    
    uint16_t result = dest_val + src_val;

    //Set value
    setRegisterValue(cpu, instruction->first_operand, (uint8_t)result);

    //Check flags
    clearFlag(cpu, SUB); //Subtraction flag
    updateFlag(cpu, ZERO, CHECK_ZERO_8(result));
    updateFlag(cpu, CARRY, CHECK_CARRY_8BIT_ADD(result));
    updateFlag(cpu, HALFCARRY, CHECK_HALF_CARRY_8BIT_ADD(dest_val, src_val));

    //4 t-cycles
    return 4;
}

//Adds value from 8-bit register to value at address stored in 16-bit register and stores it in the 8-bit register
int add_r8_r16mem(CPU* cpu, Instruction* instruction) {
    //Get values
    uint16_t src_address = getRegisterValue16(cpu, instruction->second_operand);
    uint8_t dest_val = getRegisterValue8(cpu, instruction->first_operand);
    uint8_t src_val = mem_read(src_address);

    uint16_t result = src_val + dest_val;

    //Store value
    setRegisterValue(cpu, instruction->first_operand, (uint8_t)result);

    //Update flags
    clearFlag(cpu, SUB); //Subtraction flag
    updateFlag(cpu, ZERO, CHECK_ZERO_8(result)); //Zero flag
    updateFlag(cpu, CARRY, CHECK_CARRY_8BIT_ADD(result)); //Carry flag
    updateFlag(cpu, HALFCARRY, CHECK_HALF_CARRY_8BIT_ADD(src_val, dest_val));

    //8 t-cycles
    return 8;
}

//Add value in 8-bit regsiter to 8-bit immediate and store it in the 8-bit regsiter
int add_r8_imm8(CPU* cpu, Instruction* instruction) {
    //Get values
    uint8_t src_val = mem_read(cpu->registers.pc++);
    uint8_t dest_val = getRegisterValue8(cpu, instruction->first_operand);
    
    uint16_t result = src_val + dest_val;

    //Store value
    setRegisterValue(cpu, instruction->first_operand, (uint8_t)result);

    //Update flags
    clearFlag(cpu, SUB);
    updateFlag(cpu, ZERO, CHECK_ZERO_8(result));
    updateFlag(cpu, CARRY, CHECK_CARRY_8BIT_ADD(result));
    updateFlag(cpu, HALFCARRY, CHECK_HALF_CARRY_8BIT_ADD(src_val, dest_val));

    //8 t-cycles
    return 8;
}

//Add values of 2 16-bit registers and stores it in the first one
int add_r16_r16(CPU* cpu, Instruction* instruction) {
    //Get values
    uint16_t src_val = getRegisterValue16(cpu, instruction->second_operand);
    uint16_t dest_val = getRegisterValue16(cpu, instruction->first_operand);

    uint32_t result = src_val + dest_val;

    //Store value
    setRegisterValue(cpu, instruction->first_operand, (uint16_t)result);

    //Check flags
    clearFlag(cpu, SUB);
    updateFlag(cpu, CARRY, CHECK_CARRY_16BIT_ADD(result));
    updateFlag(cpu, HALFCARRY, CHECK_HALF_CARRY_16BIT_ADD(src_val, dest_val));

    //8 t-cycles
    return 8;
}

//Adds value stored in 16-bit register to signed 8-bit immediate and stores it in 16-bit register
int add_r16_imm8s(CPU* cpu, Instruction* instruction) {
    //Get values
    int8_t src_val = (int8_t)mem_read(cpu->registers.pc++);
    uint16_t dest_val = getRegisterValue16(cpu, instruction->first_operand);

    uint16_t result = dest_val + src_val;

    //Store result
    setRegisterValue(cpu, instruction->first_operand, result);

    //Check flags
    //This instruction is special as the carry and half-carry are determined by the lower byte
    clearFlag(cpu, SUB);
    clearFlag(cpu, ZERO);
    updateFlag(cpu, CARRY, ((dest_val & 0xFF) + (uint8_t)src_val) > 0xFF); //Carry flag
    updateFlag(cpu, HALFCARRY, ((dest_val & 0xF) + ((uint8_t)src_val & 0xF)) > 0xF); //Half-carry flag

    //16 t-cycles
    return 16;
}

//Adds value in 8-bit register to 8-bit register as well as the carry bit and stores in first 8-bit register
int adc_r8_r8(CPU* cpu, Instruction* instruction) {
    //Get values
    uint8_t dest_val = getRegisterValue8(cpu, instruction->first_operand);
    uint8_t src_val = getRegisterValue8(cpu, instruction->second_operand);
    int carry = flagIsSet(cpu, CARRY);

    uint16_t result = src_val + dest_val + carry;

    //Store value
    setRegisterValue(cpu, instruction->first_operand, (uint8_t)result);

    //Update flags
    clearFlag(cpu, SUB);
    updateFlag(cpu, ZERO, CHECK_ZERO_8(result));
    updateFlag(cpu, CARRY, CHECK_CARRY_8BIT_ADD(result));
    updateFlag(cpu, HALFCARRY, CHECK_HALF_CARRY_8BIT_ADD_W_CARRY(src_val, dest_val, carry));

    //4 t-cycles
    return 4;
}

//Add value from 8-bit register to carry bit and value from address stored at 16-bit register, then store the result in 8-bit register
int adc_r8_r16mem(CPU* cpu, Instruction* instruction) {
    //Get values
    uint16_t src_address = getRegisterValue16(cpu, instruction->second_operand);
    uint8_t src_val = mem_read(src_address);
    uint8_t dest_val = getRegisterValue8(cpu, instruction->first_operand);
    int carry = flagIsSet(cpu, CARRY);

    uint16_t result = src_val + dest_val + carry;

    //Store result
    setRegisterValue(cpu, instruction->first_operand, (uint8_t)result);

    //Update flags
    clearFlag(cpu, SUB);
    updateFlag(cpu, ZERO, CHECK_ZERO_8(result));
    updateFlag(cpu, CARRY, CHECK_CARRY_8BIT_ADD(result));
    updateFlag(cpu, HALFCARRY, CHECK_HALF_CARRY_8BIT_ADD_W_CARRY(src_val, dest_val, carry));

    //8 t-cycles
    return 8;
}

//Adds value in 8 bit register to immediate 8 bit value and carry and stores value in 8-bit register
int adc_r8_imm8(CPU* cpu, Instruction* instruction) {
    //Get values
    uint8_t src_val = mem_read(cpu->registers.pc++);
    uint8_t dest_val = getRegisterValue8(cpu, instruction->first_operand);
    int carry = flagIsSet(cpu, CARRY);

    uint16_t result = src_val + dest_val + carry;

    //Store value
    setRegisterValue(cpu, instruction->first_operand, (uint8_t)result);

    //Update flags
    clearFlag(cpu, SUB);
    updateFlag(cpu, ZERO, CHECK_ZERO_8(result));
    updateFlag(cpu, CARRY, CHECK_CARRY_8BIT_ADD(result));
    updateFlag(cpu, HALFCARRY, CHECK_HALF_CARRY_8BIT_ADD_W_CARRY(src_val, dest_val, carry));

    //8 t-cycles
    return 8;
}

//Subtracts value in 2nd 8-bit register from value in 1st 8-bit register and stores the result in the first 8-bit register
int sub_r8_r8(CPU* cpu, Instruction* instruction) {
    //Get values
    uint8_t firstOp = getRegisterValue8(cpu, instruction->first_operand);
    uint8_t secondOp = getRegisterValue8(cpu, instruction->second_operand);

    uint8_t result = firstOp - secondOp; 

    //Store value
    setRegisterValue(cpu, instruction->first_operand, result);

    //Update flags
    setFlag(cpu, SUB);
    updateFlag(cpu, ZERO, CHECK_ZERO_8(result));
    updateFlag(cpu, CARRY, CHECK_CARRY_8BIT_SUB(firstOp, secondOp));
    updateFlag(cpu, HALFCARRY, CHECK_HALF_CARRY_8BIT_SUB(firstOp, secondOp));

    //4 t-cycles
    return 4;
}

//Subtracts value at address stored in 16-bit register from 8-bit register value, then stores result in 8-bit register
int sub_r8_r16mem(CPU* cpu, Instruction* instruction) {
    //Get values
    uint16_t src_address = getRegisterValue16(cpu, instruction->second_operand);
    uint8_t secondOp = mem_read(src_address);
    uint8_t firstOp = getRegisterValue8(cpu, instruction->first_operand);

    uint8_t result = firstOp - secondOp; 

    //Store value
    setRegisterValue(cpu, instruction->first_operand, result);

    //Update flags
    setFlag(cpu, SUB);
    updateFlag(cpu, ZERO, CHECK_ZERO_8(result));
    updateFlag(cpu, CARRY, CHECK_CARRY_8BIT_SUB(firstOp, secondOp));
    updateFlag(cpu, HALFCARRY, CHECK_HALF_CARRY_8BIT_SUB(firstOp, secondOp));

    //8 t-cycles
    return 8;
}

//Subtracts 8-bit immediate from value in 8-bit regsiter and stores the result in 8-bit register
int sub_r8_imm8(CPU* cpu, Instruction* instruction) {
    //Get values
    uint8_t firstOperand = getRegisterValue8(cpu, instruction->first_operand);
    uint8_t secondOperand = mem_read(cpu->registers.pc++);

    uint8_t result = firstOperand - secondOperand;

    //Store value
    setRegisterValue(cpu, instruction->first_operand, result);

    //Update flags
    setFlag(cpu, SUB);
    updateFlag(cpu, ZERO, CHECK_ZERO_8(result));
    updateFlag(cpu, CARRY, CHECK_CARRY_8BIT_SUB(firstOperand, secondOperand));
    updateFlag(cpu, HALFCARRY, CHECK_HALF_CARRY_8BIT_SUB(firstOperand, secondOperand));

    //8 t-cycles
    return 8;
}

//Subtraction from 8-bit register with carry
int sbc_r8_r8(CPU* cpu, Instruction* instruction) {
    //Get values
    uint8_t firstOp = getRegisterValue8(cpu, instruction->first_operand);
    uint8_t secondOp = getRegisterValue8(cpu, instruction->second_operand);
    int carry = flagIsSet(cpu, CARRY);

    uint8_t result = firstOp - secondOp - carry;

    //Store result
    setRegisterValue(cpu, instruction->first_operand, result);

    //Update flags
    setFlag(cpu, SUB);
    updateFlag(cpu, ZERO, CHECK_ZERO_8(result));
    updateFlag(cpu, CARRY, CHECK_CARRY_8BIT_SUB_W_CARRY(firstOp, secondOp, carry));
    updateFlag(cpu, HALFCARRY, CHECK_HALF_CARRY_8BIT_SUB_W_CARRY(firstOp, secondOp, carry));

    //4 t-cycles
    return 4;
}

//Subtracts 8-bit value at address stored in 16-bit register with carry then stores the result back
int sbc_r8_r16mem(CPU* cpu, Instruction* instruction) {
    //Get values
    uint16_t src_address = getRegisterValue16(cpu, instruction->second_operand);
    uint8_t firstOp = getRegisterValue8(cpu, instruction->first_operand);
    uint8_t secondOp = mem_read(src_address);
    int carry = flagIsSet(cpu, CARRY);

    uint8_t result = firstOp - secondOp - carry;

    //Store result
    setRegisterValue(cpu, instruction->first_operand, result);

    //Update flags
    setFlag(cpu, SUB);
    updateFlag(cpu, ZERO, CHECK_ZERO_8(result));
    updateFlag(cpu, CARRY, CHECK_CARRY_8BIT_SUB_W_CARRY(firstOp, secondOp, carry));
    updateFlag(cpu, HALFCARRY, CHECK_HALF_CARRY_8BIT_SUB_W_CARRY(firstOp, secondOp, carry));

    //8 t-cycles
    return 8;
}

//Subtracts 8-bit immediate from value in 8-bit register then stores the result in register
int sbc_r8_imm8(CPU* cpu, Instruction* instruction) {
    //Get values
    uint8_t firstOp = getRegisterValue8(cpu, instruction->first_operand);
    uint8_t secondOp = mem_read(cpu->registers.pc++);
    int carry = flagIsSet(cpu, CARRY);

    uint8_t result = firstOp - secondOp - carry;

    //Store result
    setRegisterValue(cpu, instruction->first_operand, result);

    //Update flags
    setFlag(cpu, SUB);
    updateFlag(cpu, ZERO, CHECK_ZERO_8(result));
    updateFlag(cpu, CARRY, CHECK_CARRY_8BIT_SUB_W_CARRY(firstOp, secondOp, carry));
    updateFlag(cpu, HALFCARRY, CHECK_HALF_CARRY_8BIT_SUB_W_CARRY(firstOp, secondOp, carry));

    //8 t-cycles
    return 8;
}

//Subtracts 8-bit register value from 8-bit register value and updates flags
int cp_r8_r8(CPU* cpu, Instruction* instruction) {
    //Get values
    uint8_t firstOp = getRegisterValue8(cpu, instruction->first_operand);
    uint8_t secondOp = getRegisterValue8(cpu, instruction->second_operand);

    //Update flags
    setFlag(cpu, SUB);
    updateFlag(cpu, ZERO, CHECK_ZERO_8(firstOp - secondOp));
    updateFlag(cpu, CARRY, CHECK_CARRY_8BIT_SUB(firstOp, secondOp));
    updateFlag(cpu, HALFCARRY, CHECK_HALF_CARRY_8BIT_SUB(firstOp, secondOp));

    //4 t-cycles
    return 4;
}

//Subtracts value at address stored in 16-bit regstier from 8-bit register and updates flags
int cp_r8_r16mem(CPU* cpu, Instruction* instruction) {
    //Get values
    uint8_t firstOp = getRegisterValue8(cpu, instruction->first_operand);
    uint16_t src_address = getRegisterValue16(cpu, instruction->second_operand);
    uint8_t secondOp = mem_read(src_address);

    //Update flags
    setFlag(cpu, SUB);
    updateFlag(cpu, ZERO, CHECK_ZERO_8(firstOp - secondOp));
    updateFlag(cpu, CARRY, CHECK_CARRY_8BIT_SUB(firstOp, secondOp));
    updateFlag(cpu, HALFCARRY, CHECK_HALF_CARRY_8BIT_SUB(firstOp, secondOp));

    //8 t-cycles
    return 8;
}

//Subtracts 8-bit immediate from value stored in 8-bit register and updates flags
int cp_r8_imm8(CPU* cpu, Instruction* instruction) {
    //Get values
    uint8_t firstOp = getRegisterValue8(cpu, instruction->first_operand);
    uint8_t secondOp = mem_read(cpu->registers.pc++);

    //Update flags
    setFlag(cpu, SUB);
    updateFlag(cpu, ZERO, CHECK_ZERO_8(firstOp - secondOp));
    updateFlag(cpu, CARRY, CHECK_CARRY_8BIT_SUB(firstOp, secondOp));
    updateFlag(cpu, HALFCARRY, CHECK_HALF_CARRY_8BIT_SUB(firstOp, secondOp));

    //8 t-cycles
    return 8;
}