#include "instructions.h"

//Loads 16-bit immediate from memory into 16-bit register
int ld_r16_imm16(CPU* cpu, Instruction* instruction) {
    //Get immediate from memory
    uint8_t src_lsb = mem_read(cpu->memory, cpu->registers.pc++, CPU_ACCESS);
    uint8_t src_msb = mem_read(cpu->memory, cpu->registers.pc++, CPU_ACCESS);

    //Store in register
    setRegisterValue(cpu, instruction->first_operand, UNSIGNED_16(src_lsb, src_msb));
    
    //This operation takes 12 t-states.
    return 12;
}

//Loads value in 8-bit register to address stored in 16-bit register
int ld_r16mem_r8(CPU* cpu, Instruction* instruction) {
    //Get values from registers
    uint16_t dest_address = getRegisterValue16(cpu, instruction->first_operand);
    uint8_t src_val = getRegisterValue8(cpu, instruction->second_operand);

    //Write byte from r8 into address at r16
    mem_write(cpu->memory, dest_address, src_val, CPU_ACCESS);

    //This takes 8 t-states
    return 8;
}

//Loads 8-bit immediate from memory into 8-bit register
int ld_r8_imm8(CPU* cpu, Instruction* instruction) {
    //Get immediate from memory
    uint8_t src_val = mem_read(cpu->memory, cpu->registers.pc++, CPU_ACCESS);

    //Store value in register
    setRegisterValue(cpu, instruction->first_operand, src_val);

    //This takes 8 t-states
    return 8;
}

//Loads 8-bit immediate from memory into memory at address stored in 16-bit register
int ld_r16mem_imm8(CPU* cpu, Instruction* instruction) {
    //Get values
    uint16_t dest_address = getRegisterValue16(cpu, instruction->first_operand); //from register
    uint8_t src_val = mem_read(cpu->memory, cpu->registers.pc++, CPU_ACCESS); //From memory

    //Writes value
    mem_write(cpu->memory, dest_address, src_val, CPU_ACCESS);

    //This takes 12 t-states
    return 12;
}

//Loads value from 16-bit register into 16-bit immediate address
int ld_mem_r16(CPU* cpu, Instruction* instruction) {
    //Get values
    uint8_t dest_address_lsb = mem_read(cpu->memory, cpu->registers.pc++, CPU_ACCESS);
    uint8_t dest_address_msb = mem_read(cpu->memory, cpu->registers.pc++, CPU_ACCESS);
    uint16_t dest_address = UNSIGNED_16(dest_address_lsb, dest_address_msb);
    uint16_t src_val = getRegisterValue16(cpu, instruction->first_operand);

    //Write values
    mem_write(cpu->memory, dest_address++, GET_LSB(src_val), CPU_ACCESS);
    mem_write(cpu->memory, dest_address, GET_MSB(src_val), CPU_ACCESS);

    //This takes 20 t-states
    return 20;
}

//Loads value from address stored in 16-bit register into 8-bit register
int ld_r8_r16mem(CPU* cpu, Instruction* instruction) {
    //Get value
    uint16_t src_address = getRegisterValue16(cpu, instruction->second_operand);
    
    //Store value
    setRegisterValue(cpu, instruction->first_operand, mem_read(cpu->memory, src_address, CPU_ACCESS));

    //This takes 8 t-states
    return 8;
}

//Loads value from 8 bit register into 8 bit register
int ld_r8_r8(CPU* cpu, Instruction* instruction) {
    //Get values
    uint8_t src_val = getRegisterValue8(cpu, instruction->second_operand);

    //Write value
    setRegisterValue(cpu, instruction->first_operand, src_val);

    //This takes 4 t-states
    return 4;
}

//Loads value from immediate address into 8-bit register
int ld_r8_mem(CPU* cpu, Instruction* instruction) {
    //Get values
    uint8_t src_address_lsb = mem_read(cpu->memory, cpu->registers.pc++, CPU_ACCESS);
    uint8_t src_address_msb = mem_read(cpu->memory, cpu->registers.pc++, CPU_ACCESS);
    uint8_t src_val = mem_read(cpu->memory, UNSIGNED_16(src_address_lsb, src_address_msb), CPU_ACCESS);

    //Store value
    setRegisterValue(cpu, instruction->first_operand, src_val);

    //16 t-cycles
    return 16;
}

//Loads value from 8 bit register into 16-bit immediate address
int ld_mem_r8(CPU* cpu, Instruction* instruction) {
    //Get values
    uint8_t dest_address_lsb = mem_read(cpu->memory, cpu->registers.pc++, CPU_ACCESS);
    uint8_t dest_address_msb = mem_read(cpu->memory, cpu->registers.pc++, CPU_ACCESS);
    uint8_t src_val = getRegisterValue8(cpu, instruction->first_operand);

    //Set value
    mem_write(cpu->memory, UNSIGNED_16(dest_address_lsb, dest_address_msb), src_val, CPU_ACCESS);

    //16 t-cycles
    return 16;
}

//Loads value from 16-bit register into 16-bit register
int ld_r16_r16(CPU* cpu, Instruction* instruction) {
    //Get Values
    uint16_t src_val = getRegisterValue16(cpu, instruction->second_operand);

    //Store value
    setRegisterValue(cpu, instruction->first_operand, src_val);

    //8 t-cycles
    return 8;
}

//Loads sp + signed 8-bit value into 16-bit register
int ld_r16_imm8s(CPU* cpu, Instruction* instruction) {
    //Get values
    int8_t src_offset = (int8_t)(mem_read(cpu->memory, cpu->registers.pc++, CPU_ACCESS));
    uint16_t src_val = cpu->registers.sp + src_offset;

    //Store value
    setRegisterValue(cpu, instruction->first_operand, src_val);

    //Update flags
    clearFlag(cpu, ZERO);
    clearFlag(cpu, SUB);
    //This instruction does the signed addition, so it only checks the lower byte of the unsigned addition
    //So this masks the operands to 8/4 bits and checks for overflow and updates accordingly
    updateFlag(cpu, CARRY, ((cpu->registers.sp & 0xFF) + (uint8_t)src_offset) > 0xFF);
    updateFlag(cpu, HALFCARRY, ((cpu->registers.sp & 0xF) + ((uint8_t)src_offset & 0xF)) > 0xF);

    //12 t-cycles
    return 12;
}

//Loads value from 8 bit register into memory address stored in 16-bit register, then increments the register
int ld_r16meminc_r8(CPU* cpu, Instruction* instruction) {
    //Get values
    uint16_t dest_address = getRegisterValue16(cpu, instruction->first_operand);
    uint8_t src_val = getRegisterValue8(cpu, instruction->second_operand);

    //Set value
    mem_write(cpu->memory, dest_address, src_val, CPU_ACCESS);

    //Increment register
    setRegisterValue(cpu, instruction->first_operand, dest_address + 1);

    //8 t-cycles
    return 8;
}

//Loads value from 8-bit register into address stored in 16-bit register, then decrements the 16-bit register
int ld_r16memdec_r8(CPU* cpu, Instruction* instruction) {
    //Get values
    uint16_t dest_address = getRegisterValue16(cpu, instruction->first_operand);
    uint8_t src_val = getRegisterValue8(cpu, instruction->second_operand);

    //Set value
    mem_write(cpu->memory, dest_address, src_val, CPU_ACCESS);

    //Decrement register
    setRegisterValue(cpu, instruction->first_operand, dest_address - 1);

    //8 t-cycles
    return 8;
}

//Loads value from memory address in 16-bit register into 8-bit register, then increments the 16-bit register
int ld_r8_r16meminc(CPU* cpu, Instruction* instruction) {
    //Get values
    uint16_t src_address = getRegisterValue16(cpu, instruction->second_operand);
    uint8_t src_val = mem_read(cpu->memory, src_address, CPU_ACCESS);

    //Store value
    setRegisterValue(cpu, instruction->first_operand, src_val);

    //Increment register
    setRegisterValue(cpu, instruction->second_operand, src_address + 1);

    //8 t-cycles
    return 8;
}

//Loads value from address in 16-bit register into 8-bit register, then decrements the 16-bit register
int ld_r8_r16memdec(CPU* cpu, Instruction* instruction) {
    //Get values
    uint16_t src_address = getRegisterValue16(cpu, instruction->second_operand);
    uint8_t src_val = mem_read(cpu->memory, src_address, CPU_ACCESS);

    //Store value
    setRegisterValue(cpu, instruction->first_operand, src_val);

    //Decrement register
    setRegisterValue(cpu, instruction->second_operand, src_address - 1);

    //8 t-cycles
    return 8;
}

//Loads value from 8-bit register into memory address at 0xFF00 + 8-bit immediate
int ldh_mem_r8(CPU* cpu, Instruction* instruction) {
    //Get values
    uint8_t dest_address_lsb = mem_read(cpu->memory, cpu->registers.pc++, CPU_ACCESS);
    uint8_t src_val = getRegisterValue8(cpu, instruction->first_operand);

    //Write value
    mem_write(cpu->memory, UNSIGNED_16(dest_address_lsb, 0xFF), src_val, CPU_ACCESS);

    //12 t-cycles
    return 12;
}

//Loads value from memory at 0xFF00 + 8-bit immediate into 8-bit register
int ldh_r8_mem(CPU* cpu, Instruction* instruction) {
    //Get values
    uint8_t src_address_lsb = mem_read(cpu->memory, cpu->registers.pc++, CPU_ACCESS);
    uint8_t src_val = mem_read(cpu->memory, UNSIGNED_16(src_address_lsb, 0xFF), CPU_ACCESS);

    //Set value
    setRegisterValue(cpu, instruction->first_operand, src_val);

    //12 t-cycles
    return 12;
}

//Loads value from 8-bit register into address at 0xFF00 + value in 8-bit register
int ldh_r8mem_r8(CPU* cpu, Instruction* instruction) {
    //Get values
    uint8_t src_val = getRegisterValue8(cpu, instruction->second_operand);
    uint8_t dest_address_lsb = getRegisterValue8(cpu, instruction->first_operand);

    //Write value
    mem_write(cpu->memory, UNSIGNED_16(dest_address_lsb, 0xFF), src_val, CPU_ACCESS);

    //8 t-cycles
    return 8;
}

//Loads value from address at 0xFF00 + value stored in 8-bit register into 8-bit register
int ldh_r8_r8mem(CPU* cpu, Instruction* instruction) {
    //Get values
    uint8_t src_address_lsb = getRegisterValue8(cpu, instruction->second_operand);
    uint8_t src_val = mem_read(cpu->memory, UNSIGNED_16(src_address_lsb, 0xFF), CPU_ACCESS);

    //Write value
    setRegisterValue(cpu, instruction->first_operand, src_val);

    //8 t-cycles
    return 8;
}

//Pushes value of 16-bit register onto stack
int push(CPU* cpu, Instruction* instruction) {
    //Get value
    uint16_t src_val = getRegisterValue16(cpu, instruction->first_operand);

    //Write value onto stack
    mem_write(cpu->memory, --cpu->registers.sp, GET_MSB(src_val), CPU_ACCESS);
    mem_write(cpu->memory, --cpu->registers.sp, GET_LSB(src_val), CPU_ACCESS);

    //16 t-cycles
    return 16;
}

//Pops value off stack into 16-bit register
int pop(CPU* cpu, Instruction* instruction) {
    //Get values from stack
    uint8_t src_val_lsb = mem_read(cpu->memory, cpu->registers.sp++, CPU_ACCESS);
    uint8_t src_val_msb = mem_read(cpu->memory, cpu->registers.sp++, CPU_ACCESS);

    //Write values
    setRegisterValue(cpu, instruction->first_operand, UNSIGNED_16(src_val_lsb, src_val_msb));

    //12 t-cycles
    return 12;
}