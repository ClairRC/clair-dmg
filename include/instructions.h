#include "cpu.h"
#include <stdint.h>
#include <stdlib.h>

//Macros to help with writing instructions
#define UNSIGNED_16(lsb, msb) (((uint16_t)(msb)<<8) | (uint16_t)(lsb)) //Combines 2 bytes
#define GET_LSB(val) ((uint8_t)(val)) //Truncates upper byte, leaving lsb
#define GET_MSB(val) ((uint8_t) ((val)>>8)) //Shifts bits over and truncates upper bits, leaving msb

//Checks flags.
#define CHECK_ZERO_8(val) ((uint8_t)(val) == 0)
#define CHECK_ZERO_16(val) ((uint16_t)(val) == 0)
#define CHECK_CARRY_8BIT_ADD(val_16) ((val_16) > 0xFF)
#define CHECK_CARRY_16BIT_ADD(val_32) ((val_32) > 0xFFFF)
#define CHECK_HALF_CARRY_8BIT_ADD(val1_8, val2_8) (((val1_8) & 0xF) + ((val2_8) & 0xF) > 0xF)
#define CHECK_HALF_CARRY_16BIT_ADD(val1_16, val2_16) (((val1_16) & 0xFFF) + ((val2_16) & 0xFFF) > 0xFFF)
#define CHECK_HALF_CARRY_8BIT_ADD_W_CARRY(val1_8, val2_8, carryBit)\
    (((val1_8) & 0xF) + ((val2_8) & 0xF) + carryBit > 0xF)
#define CHECK_CARRY_8BIT_SUB(val1_8, val2_8) ((val1_8) < (val2_8))
#define CHECK_CARRY_8BIT_SUB_W_CARRY(val1_8, val2_8, carryBit) ((val1_8) < ((uint16_t)(val2_8) + carryBit))
#define CHECK_HALF_CARRY_8BIT_SUB(val1_8, val2_8) (((val1_8) & 0xF) < ((val2_8) & 0xF))
#define CHECK_HALF_CARRY_8BIT_SUB_W_CARRY(val1_8, val2_8, carryBit) ((val1_8 & 0xF) < (((val2_8) & 0xF) + carryBit))

/*
* Because there are so many opcodes, I've tried my best to only include
* a few function per type. There will still be a lot of functions, but because
* of different operand types, I've still had to include a lot.
* I was struggling between "include functions at are as specific as possible" and
* "make my code clean and readable", and so ultimately I decided I will have a different
* function for different operand types, including for special cases. So there's a lot,
* but not as many as there are opcodes.
*/

//Instruction struct

//First operand always comes from instruction, the function fetches the rest if necessary
//If the instruction has 2 operands, then they'll be in order of dest/src
typedef struct Instruction Instruction; //Forward declaration
struct Instruction {
    int (*funct_ptr)(CPU*, Instruction*); //Pointer to function
    
    //Operands.
    uint8_t first_operand;
    uint8_t second_operand;

    size_t num_bytes; //Size of instruction in bytes
};

//Instruction functions
//Returns number of T-states instruction took

//Load instructions             //dest|src
int ld_r16_imm16(CPU*, Instruction*); //16-bit register|16-bit immediate
int ld_r16mem_r8(CPU*, Instruction*); //[16-bit register]|8-bit register
int ld_r8_imm8(CPU*, Instruction*); //8-bit register|8-bit immediate
int ld_r16mem_imm8(CPU*, Instruction*); //[16-bit register]|8-bit immediate
int ld_mem_r16(CPU*, Instruction*); //[16-bit address]|16-bit register
int ld_r8_r16mem(CPU*, Instruction*); //8-bit register|[16-bit register]
int ld_r8_r8(CPU*, Instruction*); //8-bit register|8-bit register
int ld_r8_mem(CPU*, Instruction*); //8-bit register|[16-bit address]
int ld_mem_r8(CPU*, Instruction*);

int ld_r16_r16(CPU*, Instruction*); //16-bit register|16-bit register
int ld_r16_imm8s(CPU*, Instruction*); //16-bit register|SP + 8-bit signed immediate

int ld_r16meminc_r8(CPU*, Instruction*); //[16-bit register] (inc register)|8-bit register
int ld_r16memdec_r8(CPU*, Instruction*); //[16-bit register] (dec register)|8-bit register
int ld_r8_r16meminc(CPU*, Instruction*); //8-bit register|[16-bit register] (inc register)
int ld_r8_r16memdec(CPU*, Instruction*); //8-bit register|[16-bit register] (dec register)

int ldh_mem_r8(CPU*, Instruction*); //[0xFF00 + 8-bit address]|8-bit register
int ldh_r8_mem(CPU*, Instruction*); //8-bit register|[0xFF00 + 8-bit address]
int ldh_r8mem_r8(CPU*, Instruction*); //[0xFF00 + 8-bit register]|8-bit register
int ldh_r8_r8mem(CPU*, Instruction*); //8-bit register|[0xFF00 + 8-bit register]

//Push/Pop instructions
int push(CPU*, Instruction*);
int pop(CPU*, Instruction*);

//Arithmetic Instructions
int add_r8_r8(CPU*, Instruction*);
int add_r8_r16mem(CPU*, Instruction*);
int add_r8_imm8(CPU*, Instruction*);
int add_r16_r16(CPU*, Instruction*);
int add_r16_imm8s(CPU*, Instruction*); //Adds SP to signed int

int adc_r8_r8(CPU*, Instruction*);
int adc_r8_r16mem(CPU*, Instruction*);
int adc_r8_imm8(CPU*, Instruction*);

int sub_r8_r8(CPU*, Instruction*);
int sub_r8_r16mem(CPU*, Instruction*);
int sub_r8_imm8(CPU*, Instruction*);

int sbc_r8_r8(CPU*, Instruction*);
int sbc_r8_r16mem(CPU*, Instruction*);
int sbc_r8_imm8(CPU*, Instruction*);

int cp_r8_r8(CPU*, Instruction*);
int cp_r8_r16mem(CPU*, Instruction*);
int cp_r8_imm8(CPU*, Instruction*);

//Bitwise Instructions
int and_r8_r8(CPU*, Instruction*);
int and_r8_r16mem(CPU*, Instruction*);
int and_r8_imm8(CPU*, Instruction*);

int xor_r8_r8(CPU*, Instruction*);
int xor_r8_r16mem(CPU*, Instruction*);
int xor_r8_imm8(CPU*, Instruction*);

int or_r8_r8(CPU*, Instruction*);
int or_r8_r16mem(CPU*, Instruction*);
int or_r8_imm8(CPU*, Instruction*);

//Increment/Decrement
int inc_r16(CPU*, Instruction*);
int inc_r8(CPU*, Instruction*);
int inc_r16mem(CPU*, Instruction*);
int dec_r16(CPU*, Instruction*);
int dec_r8(CPU*, Instruction*);
int dec_r16mem(CPU*, Instruction*);

//Accumulator rotation
int rrca(CPU*, Instruction*); //rotate right circular
int rra(CPU*, Instruction*); //rotate right, rotate in carry bit
int rlca(CPU*, Instruction*); //rotate left circular
int rla(CPU*, Instruction*); //rotate left, rotate in carry bit

//Call instructions
int call_imm16(CPU*, Instruction*); //Calls function at imm16
int call_flag_imm16(CPU*, Instruction*); //Calls function at imm16 if flag is set

int rst_imm16(CPU*, Instruction*); //Jump to hard coded addresses

//Absolute Jump
int jp_imm16(CPU*, Instruction*); //Unconditional jump to address
int jp_flag_imm16(CPU*, Instruction*); //Jump if flag is set
int jp_hl(CPU*, Instruction*); //Outlier that jumps to the address stored in HL

//Relative Jump
int jr_imm8s(CPU*, Instruction*); //Unconditionaly
int jr_flag_imm8s(CPU*, Instruction*); //If flag

//Return Instructions
int ret(CPU*, Instruction*); //Unconditional
int ret_flag(CPU*, Instruction*); //Return if flag is set

int reti(CPU*, Instruction*); //Return from interrupt handler

//Misc instructions (wasnt sure how to caregorize these)
int cpl(CPU*, Instruction*); //Complement accumulator (A register)
int ccf(CPU*, Instruction*); //Complement carry flag
int scf(CPU*, Instruction*); //Set carry flag
int daa(CPU*, Instruction*); //Decimal adjust accumulator

int stop(CPU*, Instruction*);
int halt(CPU*, Instruction*);
int nop(CPU*, Instruction*);

int di(CPU*, Instruction*);
int ei(CPU*, Instruction*);


//CB Instructions
//The GameBoy has different instructions for the CB prefix, so I'm going 
//to handle these separately, but they are really kinda nothing new.
int rlc_r(CPU*, Instruction*);
int rrc_r(CPU*, Instruction*);
int rl_r(CPU*, Instruction*);
int rr_r(CPU*, Instruction*);
int sla_r(CPU*, Instruction*);
int sra_r(CPU*, Instruction*);
int swap_r(CPU*, Instruction*);
int srl_r(CPU*, Instruction*);
int bit_b_r(CPU*, Instruction*);
int res_b_r(CPU*, Instruction*);
int set_b_r(CPU*, Instruction*);