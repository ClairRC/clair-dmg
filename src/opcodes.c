#include <stdint.h>
#include "instructions.h"

#define OP(opcode, function, first_op, second_op, size) [opcode] =\
            (Instruction){.funct_ptr = function, .first_operand = first_op,\
            .second_operand=second_op, .num_bytes=size}\

//Lookup tables for CPU instructions.
Instruction main_instructions[256] = {
    //0x00-0x0F
    OP(0x00, nop, 0, 0, 1),
    OP(0x01, ld_r16_imm16, REG_BC, 0, 3),
    OP(0x02, ldS_r16mem_r8, REG_BC, REG_A, 1),
    OP(0x03, inc_r16, REG_BC, 0, 1),
    OP(0x04, inc_r8, REG_B, 0, 1),
    OP(0x05, dec_r8, REG_B, 0, 1),
    OP(0x06, ld_r8_imm8, REG_B, 0, 2),
    OP(0x07, rlca, 0, 0, 1),
    OP(0x08, ld_mem_r16, REG_sp, 0, 3),
    OP(0x09, add_r16_r16, REG_HL, REG_BC, 1),
    OP(0x0A, ld_r8_r16mem, REG_A, REG_BC, 1),
    OP(0x0B, dec_r16, REG_BC, 0, 1),
    OP(0x0C, inc_r8, REG_C, 0, 1),
    OP(0x0D, dec_r8, REG_C, 0, 1),
    OP(0x0E, ld_r8_imm8, REG_C, 0, 2),
    OP(0x0F, rrca, 0, 0, 1),
};