#include <stdint.h>
#include "instructions.h"

//Main instruction macro
#define OP(opcode, function, first_op, second_op, size, cycles) main_instructions[opcode] =\
            (Instruction){.funct_ptr = function, .first_operand = first_op,\
            .second_operand=second_op, .num_bytes=size, .min_num_cycles = cycles}\

//CB instruction macro
#define CB_OP(opcode, function, first_op, second_op, size, cycles) cb_instructions[opcode] =\
            (Instruction){.funct_ptr = function, .first_operand = first_op,\
            .second_operand=second_op, .num_bytes=size, .min_num_cycles = cycles}\

/*
* The Gameboy has 2 instruction tables, and so I've incuded the regular ones and the "CB prefix" ones separately.
* I decided to just do a lookup table to just manually assigne each opcode to the correct function, but this
* was genuinely so tedious, so if I do something like this again, I'd either write a script to do this for me (smart)
* or I'd just handle the decoding at runtime instead, which would maybe be less performant but probably just cleaner.
* Regardless this is done so I'm gonna just use this for now.
*/

Instruction main_instructions[256];
Instruction cb_instructions[256];

void init_opcodes() {

    /* * * * * * * * * *
    *                   *
    *   MAIN OPCODES    *
    *                   *
    * * * * * * * * * * */
    //Misc / Control instructions
    OP(0x00, nop, 0, 0, 1, 4);
    OP(0x10, stop, 0, 0, 2, 4);
    OP(0x76, halt, 0, 0, 1, 4);
    OP(0xF3, di, 0, 0, 1, 4);
    OP(0xFB, ei, 0, 0, 1, 4);

    //Jumps / Calls
    OP(0x18, jr_imm8s, 0, 0, 2, 12);
    OP(0x20, jr_flag_imm8s, ZERO, 0, 2, 8);
    OP(0x28, jr_flag_imm8s, ZERO, 1, 2, 8);
    OP(0x30, jr_flag_imm8s, CARRY, 0, 2, 8);
    OP(0x38, jr_flag_imm8s, CARRY, 1, 2, 8);
    OP(0xC0, ret_flag, ZERO, 0, 1, 8);
    OP(0xC2, jp_flag_imm16, ZERO, 0, 3, 12);
    OP(0xC3, jp_imm16, 0, 0, 3, 16);
    OP(0xC4, call_flag_imm16, ZERO, 0, 3, 12);
    OP(0xC7, rst_imm16, 0x00, 0, 1, 16);
    OP(0xC8, ret_flag, ZERO, 1, 1, 8);
    OP(0xC9, ret, 0, 0, 1, 16);
    OP(0xCA, jp_flag_imm16, ZERO, 1, 3, 12);
    OP(0xCC, call_flag_imm16, ZERO, 1, 3, 12);
    OP(0xCD, call_imm16, 0, 0, 3, 24);
    OP(0xCF, rst_imm16, 0x08, 0, 1, 16);
    OP(0xD0, ret_flag, CARRY, 0, 1, 8);
    OP(0xD2, jp_flag_imm16, CARRY, 0, 3, 12);
    OP(0xD4, call_flag_imm16, CARRY, 0, 3, 12);
    OP(0xD7, rst_imm16, 0x10, 0, 1, 16);
    OP(0xD8, ret_flag, CARRY, 1, 1, 8);
    OP(0xD9, reti, 0, 0, 1, 16);
    OP(0xDA, jp_flag_imm16, CARRY, 1, 3, 12);
    OP(0xDC, call_flag_imm16, CARRY, 1, 3, 12);
    OP(0xDF, rst_imm16, 0x18, 0, 1, 16);
    OP(0xE7, rst_imm16, 0x20, 0, 1, 16);
    OP(0xE9, jp_hl, 0, 0, 1, 4);
    OP(0xEF, rst_imm16, 0x28, 0, 1, 16);
    OP(0xF7, rst_imm16, 0x30, 0, 1, 16);
    OP(0xFF, rst_imm16, 0x38, 0, 1, 16);

    //8-bit load instructions
    OP(0x02, ld_r16mem_r8, REG_BC, REG_A, 1, 8);
    OP(0x06, ld_r8_imm8, REG_B, 0, 2, 8);
    OP(0x0A, ld_r8_r16mem, REG_A, REG_BC, 1, 8);
    OP(0x0E, ld_r8_imm8, REG_C, 0, 2, 8);
    OP(0x12, ld_r16mem_r8, REG_DE, REG_A, 1, 8);
    OP(0x16, ld_r8_imm8, REG_D, 0, 2, 8);
    OP(0x1A, ld_r8_r16mem, REG_A, REG_DE, 1, 8);
    OP(0x1E, ld_r8_imm8, REG_E, 0, 2, 8);
    OP(0x22, ld_r16meminc_r8, REG_HL, REG_A, 1, 8);
    OP(0x26, ld_r8_imm8, REG_H, 0, 2, 8);
    OP(0x2A, ld_r8_r16meminc, REG_A, REG_HL, 1, 8);
    OP(0x2E, ld_r8_imm8, REG_L, 0, 2, 8);
    OP(0x32, ld_r16memdec_r8, REG_HL, REG_A, 1, 8);
    OP(0x36, ld_r16mem_imm8, REG_HL, 0, 2, 12);
    OP(0x3A, ld_r8_r16memdec, REG_A, REG_HL, 1, 8);
    OP(0x3E, ld_r8_imm8, REG_A, 0, 2, 8);
    OP(0x40, ld_r8_r8, REG_B, REG_B, 1, 4);
    OP(0x41, ld_r8_r8, REG_B, REG_C, 1, 4);
    OP(0x42, ld_r8_r8, REG_B, REG_D, 1, 4);
    OP(0x43, ld_r8_r8, REG_B, REG_E, 1, 4);
    OP(0x44, ld_r8_r8, REG_B, REG_H, 1, 4);
    OP(0x45, ld_r8_r8, REG_B, REG_L, 1, 4);
    OP(0x46, ld_r8_r16mem, REG_B, REG_HL, 1, 8);
    OP(0x47, ld_r8_r8, REG_B, REG_A, 1, 4);
    OP(0x48, ld_r8_r8, REG_C, REG_B, 1, 4);
    OP(0x49, ld_r8_r8, REG_C, REG_C, 1, 4);
    OP(0x4A, ld_r8_r8, REG_C, REG_D, 1, 4);
    OP(0x4B, ld_r8_r8, REG_C, REG_E, 1, 4);
    OP(0x4C, ld_r8_r8, REG_C, REG_H, 1, 4);
    OP(0x4D, ld_r8_r8, REG_C, REG_L, 1, 4);
    OP(0x4E, ld_r8_r16mem, REG_C, REG_HL, 1, 8);
    OP(0x4F, ld_r8_r8, REG_C, REG_A, 1, 4);
    OP(0x50, ld_r8_r8, REG_D, REG_B, 1, 4);
    OP(0x51, ld_r8_r8, REG_D, REG_C, 1, 4);
    OP(0x52, ld_r8_r8, REG_D, REG_D, 1, 4);
    OP(0x53, ld_r8_r8, REG_D, REG_E, 1, 4);
    OP(0x54, ld_r8_r8, REG_D, REG_H, 1, 4);
    OP(0x55, ld_r8_r8, REG_D, REG_L, 1, 4);
    OP(0x56, ld_r8_r16mem, REG_D, REG_HL, 1, 8);
    OP(0x57, ld_r8_r8, REG_D, REG_A, 1, 4);
    OP(0x58, ld_r8_r8, REG_E, REG_B, 1, 4);
    OP(0x59, ld_r8_r8, REG_E, REG_C, 1, 4);
    OP(0x5A, ld_r8_r8, REG_E, REG_D, 1, 4);
    OP(0x5B, ld_r8_r8, REG_E, REG_E, 1, 4);
    OP(0x5C, ld_r8_r8, REG_E, REG_H, 1, 4);
    OP(0x5D, ld_r8_r8, REG_E, REG_L, 1, 4);
    OP(0x5E, ld_r8_r16mem, REG_E, REG_HL, 1, 8);
    OP(0x5F, ld_r8_r8, REG_E, REG_A, 1, 4);
    OP(0x60, ld_r8_r8, REG_H, REG_B, 1, 4);
    OP(0x61, ld_r8_r8, REG_H, REG_C, 1, 4);
    OP(0x62, ld_r8_r8, REG_H, REG_D, 1, 4);
    OP(0x63, ld_r8_r8, REG_H, REG_E, 1, 4);
    OP(0x64, ld_r8_r8, REG_H, REG_H, 1, 4);
    OP(0x65, ld_r8_r8, REG_H, REG_L, 1, 4);
    OP(0x66, ld_r8_r16mem, REG_H, REG_HL, 1, 8);
    OP(0x67, ld_r8_r8, REG_H, REG_A, 1, 4);
    OP(0x68, ld_r8_r8, REG_L, REG_B, 1, 4);
    OP(0x69, ld_r8_r8, REG_L, REG_C, 1, 4);
    OP(0x6A, ld_r8_r8, REG_L, REG_D, 1, 4);
    OP(0x6B, ld_r8_r8, REG_L, REG_E, 1, 4);
    OP(0x6C, ld_r8_r8, REG_L, REG_H, 1, 4);
    OP(0x6D, ld_r8_r8, REG_L, REG_L, 1, 4);
    OP(0x6E, ld_r8_r16mem, REG_L, REG_HL, 1, 8);
    OP(0x6F, ld_r8_r8, REG_L, REG_A, 1, 4);
    OP(0x70, ld_r16mem_r8, REG_HL, REG_B, 1, 8);
    OP(0x71, ld_r16mem_r8, REG_HL, REG_C, 1, 8);
    OP(0x72, ld_r16mem_r8, REG_HL, REG_D, 1, 8);
    OP(0x73, ld_r16mem_r8, REG_HL, REG_E, 1, 8);
    OP(0x74, ld_r16mem_r8, REG_HL, REG_H, 1, 8);
    OP(0x75, ld_r16mem_r8, REG_HL, REG_L, 1, 8);
    OP(0x77, ld_r16mem_r8, REG_HL, REG_A, 1, 8);
    OP(0x78, ld_r8_r8, REG_A, REG_B, 1, 4);
    OP(0x79, ld_r8_r8, REG_A, REG_C, 1, 4);
    OP(0x7A, ld_r8_r8, REG_A, REG_D, 1, 4);
    OP(0x7B, ld_r8_r8, REG_A, REG_E, 1, 4);
    OP(0x7C, ld_r8_r8, REG_A, REG_H, 1, 4);
    OP(0x7D, ld_r8_r8, REG_A, REG_L, 1, 4);
    OP(0x7E, ld_r8_r16mem, REG_A, REG_HL, 1, 8);
    OP(0x7F, ld_r8_r8, REG_A, REG_A, 1, 4);
    OP(0xE0, ldh_mem_r8, REG_A, 0, 2, 12);
    OP(0xE2, ldh_r8mem_r8, REG_C, REG_A, 1, 8);
    OP(0xEA, ld_mem_r8, REG_A, 0, 3, 16);
    OP(0xF0, ldh_r8_mem, REG_A, 0, 2, 12);
    OP(0xF2, ldh_r8_r8mem, REG_A, REG_C, 1, 8);
    OP(0xFA, ld_r8_mem, REG_A, 0, 3, 16);

    //16-bit load instructions
    OP(0x01, ld_r16_imm16, REG_BC, 0, 3, 12);
    OP(0x08, ld_mem_r16, REG_sp, 0, 3, 20);
    OP(0x11, ld_r16_imm16, REG_DE, 0, 3, 12);
    OP(0x21, ld_r16_imm16, REG_HL, 0, 3, 12);
    OP(0x31, ld_r16_imm16, REG_sp, 0, 3, 12);
    OP(0xC1, pop, REG_BC, 0, 1, 12);
    OP(0xC5, push, REG_BC, 0, 1, 16);
    OP(0xD1, pop, REG_DE, 0, 1, 12);
    OP(0xD5, push, REG_DE, 0, 1, 16);
    OP(0xE1, pop, REG_HL, 0, 1, 12);
    OP(0xE5, push, REG_HL, 0, 1, 16);
    OP(0xF1, pop, REG_AF, 0, 1, 12);
    OP(0xF5, push, REG_AF, 0, 1, 16);
    OP(0xF8, ld_r16_imm8s, REG_HL, 0, 2, 12);
    OP(0xF9, ld_r16_r16, REG_sp, REG_HL, 1, 8);

    //8-bit arithmetic/logical instructions
    OP(0x04, inc_r8, REG_B, 0, 1, 4);
    OP(0x05, dec_r8, REG_B, 0, 1, 4);
    OP(0x0C, inc_r8, REG_C, 0, 1, 4);
    OP(0x0D, dec_r8, REG_C, 0, 1, 4);
    OP(0x14, inc_r8, REG_D, 0, 1, 4);
    OP(0x15, dec_r8, REG_D, 0, 1, 4);
    OP(0x1C, inc_r8, REG_E, 0, 1, 4);
    OP(0x1D, dec_r8, REG_E, 0, 1, 4);
    OP(0x24, inc_r8, REG_H, 0, 1, 4);
    OP(0x25, dec_r8, REG_H, 0, 1, 4);
    OP(0x27, daa, 0, 0, 1, 4);
    OP(0x2C, inc_r8, REG_L, 0, 1, 4);
    OP(0x2D, dec_r8, REG_L, 0, 1, 4);
    OP(0x2F, cpl, 0, 0, 1, 4);
    OP(0x34, inc_r16mem, REG_HL, 0, 1, 12);
    OP(0x35, dec_r16mem, REG_HL, 0, 1, 12);
    OP(0x37, scf, 0, 0, 1, 4);
    OP(0x3C, inc_r8, REG_A, 0, 1, 4);
    OP(0x3D, dec_r8, REG_A, 0, 1, 4);
    OP(0x3F, ccf, 0, 0, 1, 4);
    OP(0x80, add_r8_r8, REG_A, REG_B, 1, 4);
    OP(0x81, add_r8_r8, REG_A, REG_C, 1, 4);
    OP(0x82, add_r8_r8, REG_A, REG_D, 1, 4);
    OP(0x83, add_r8_r8, REG_A, REG_E, 1, 4);
    OP(0x84, add_r8_r8, REG_A, REG_H, 1, 4);
    OP(0x85, add_r8_r8, REG_A, REG_L, 1, 4);
    OP(0x86, add_r8_r16mem, REG_A, REG_HL, 1, 8);
    OP(0x87, add_r8_r8, REG_A, REG_A, 1, 4);
    OP(0x88, adc_r8_r8, REG_A, REG_B, 1, 4);
    OP(0x89, adc_r8_r8, REG_A, REG_C, 1, 4);
    OP(0x8A, adc_r8_r8, REG_A, REG_D, 1, 4);
    OP(0x8B, adc_r8_r8, REG_A, REG_E, 1, 4);
    OP(0x8C, adc_r8_r8, REG_A, REG_H, 1, 4);
    OP(0x8D, adc_r8_r8, REG_A, REG_L, 1, 4);
    OP(0x8E, adc_r8_r16mem, REG_A, REG_HL, 1, 8);
    OP(0x8F, adc_r8_r8, REG_A, REG_A, 1, 4);
    OP(0x90, sub_r8_r8, REG_A, REG_B, 1, 4);
    OP(0x91, sub_r8_r8, REG_A, REG_C, 1, 4);
    OP(0x92, sub_r8_r8, REG_A, REG_D, 1, 4);
    OP(0x93, sub_r8_r8, REG_A, REG_E, 1, 4);
    OP(0x94, sub_r8_r8, REG_A, REG_H, 1, 4);
    OP(0x95, sub_r8_r8, REG_A, REG_L, 1, 4);
    OP(0x96, sub_r8_r16mem, REG_A, REG_HL, 1, 8);
    OP(0x97, sub_r8_r8, REG_A, REG_A, 1, 4);
    OP(0x98, sbc_r8_r8, REG_A, REG_B, 1, 4);
    OP(0x99, sbc_r8_r8, REG_A, REG_C, 1, 4);
    OP(0x9A, sbc_r8_r8, REG_A, REG_D, 1, 4);
    OP(0x9B, sbc_r8_r8, REG_A, REG_E, 1, 4);
    OP(0x9C, sbc_r8_r8, REG_A, REG_H, 1, 4);
    OP(0x9D, sbc_r8_r8, REG_A, REG_L, 1, 4);
    OP(0x9E, sbc_r8_r16mem, REG_A, REG_HL, 1, 8);
    OP(0x9F, sbc_r8_r8, REG_A, REG_A, 1, 4);
    OP(0xA0, and_r8_r8, REG_A, REG_B, 1, 4);
    OP(0xA1, and_r8_r8, REG_A, REG_C, 1, 4);
    OP(0xA2, and_r8_r8, REG_A, REG_D, 1, 4);
    OP(0xA3, and_r8_r8, REG_A, REG_E, 1, 4);
    OP(0xA4, and_r8_r8, REG_A, REG_H, 1, 4);
    OP(0xA5, and_r8_r8, REG_A, REG_L, 1, 4);
    OP(0xA6, and_r8_r16mem, REG_A, REG_HL, 1, 8);
    OP(0xA7, and_r8_r8, REG_A, REG_A, 1, 4);
    OP(0xA8, xor_r8_r8, REG_A, REG_B, 1, 4);
    OP(0xA9, xor_r8_r8, REG_A, REG_C, 1, 4);
    OP(0xAA, xor_r8_r8, REG_A, REG_D, 1, 4);
    OP(0xAB, xor_r8_r8, REG_A, REG_E, 1, 4);
    OP(0xAC, xor_r8_r8, REG_A, REG_H, 1, 4);
    OP(0xAD, xor_r8_r8, REG_A, REG_L, 1, 4);
    OP(0xAE, xor_r8_r16mem, REG_A, REG_HL, 1, 8);
    OP(0xAF, xor_r8_r8, REG_A, REG_A, 1, 4);
    OP(0xB0, or_r8_r8, REG_A, REG_B, 1, 4);
    OP(0xB1, or_r8_r8, REG_A, REG_C, 1, 4);
    OP(0xB2, or_r8_r8, REG_A, REG_D, 1, 4);
    OP(0xB3, or_r8_r8, REG_A, REG_E, 1, 4);
    OP(0xB4, or_r8_r8, REG_A, REG_H, 1, 4);
    OP(0xB5, or_r8_r8, REG_A, REG_L, 1, 4);
    OP(0xB6, or_r8_r16mem, REG_A, REG_HL, 1, 8);
    OP(0xB7, or_r8_r8, REG_A, REG_A, 1, 4);
    OP(0xB8, cp_r8_r8, REG_A, REG_B, 1, 4);
    OP(0xB9, cp_r8_r8, REG_A, REG_C, 1, 4);
    OP(0xBA, cp_r8_r8, REG_A, REG_D, 1, 4);
    OP(0xBB, cp_r8_r8, REG_A, REG_E, 1, 4);
    OP(0xBC, cp_r8_r8, REG_A, REG_H, 1, 4);
    OP(0xBD, cp_r8_r8, REG_A, REG_L, 1, 4);
    OP(0xBE, cp_r8_r16mem, REG_A, REG_HL, 1, 8);
    OP(0xBF, cp_r8_r8, REG_A, REG_A, 1, 4);
    OP(0xC6, add_r8_imm8, REG_A, 0, 2, 8);
    OP(0xCE, adc_r8_imm8, REG_A, 0, 2, 8);
    OP(0xD6, sub_r8_imm8, REG_A, 0, 2, 8);
    OP(0xDE, sbc_r8_imm8, REG_A, 0, 2, 8);
    OP(0xE6, and_r8_imm8, REG_A, 0, 2, 8);
    OP(0xEE, xor_r8_imm8, REG_A, 0, 2, 8);
    OP(0xF6, or_r8_imm8, REG_A, 0, 2, 8);
    OP(0xFE, cp_r8_imm8, REG_A, 0, 2, 8);

    //16-bit arithmetic/logical instructions
    OP(0x03, inc_r16, REG_BC, 0, 1, 8);
    OP(0x09, add_r16_r16, REG_HL, REG_BC, 1, 8);
    OP(0x0B, dec_r16, REG_BC, 0, 1, 8);
    OP(0x13, inc_r16, REG_DE, 0, 1, 8);
    OP(0x19, add_r16_r16, REG_HL, REG_DE, 1, 8);
    OP(0x1B, dec_r16, REG_DE, 0, 1, 8);
    OP(0x23, inc_r16, REG_HL, 0, 1, 8);
    OP(0x29, add_r16_r16, REG_HL, REG_HL, 1, 8);
    OP(0x2B, dec_r16, REG_HL, 0, 1, 8);
    OP(0x33, inc_r16, REG_sp, 0, 1, 8);
    OP(0x39, add_r16_r16, REG_HL, REG_sp, 1, 8);
    OP(0x3B, dec_r16, REG_sp, 0, 1, 8);
    OP(0xE8, add_r16_imm8s, REG_sp, 0, 2, 16);
    

    //8-bit shift, rotate and bit instructions
    OP(0x07, rlca, 0, 0, 1, 4);
    OP(0x0F, rrca, 0, 0, 1, 4);
    OP(0x17, rla, 0, 0, 1, 4);
    OP(0x1F, rra, 0, 0, 1, 4);

    /* * * * * * * * * *
    *                   *
    *   CB OPCODES      *
    *                   *
    * * * * * * * * * * */
    //RLC
    CB_OP(0x00, rlc_r, REG_B, 0, 2, 8);
    CB_OP(0x01, rlc_r, REG_C, 0, 2, 8);
    CB_OP(0x02, rlc_r, REG_D, 0, 2, 8);
    CB_OP(0x03, rlc_r, REG_E, 0, 2, 8);
    CB_OP(0x04, rlc_r, REG_H, 0, 2, 8);
    CB_OP(0x05, rlc_r, REG_L, 0, 2, 8);
    CB_OP(0x06, rlc_r, REG_HL, 0, 2, 16);
    CB_OP(0x07, rlc_r, REG_A, 0, 2, 8);

    //RRC
    CB_OP(0x08, rrc_r, REG_B, 0, 2, 8);
    CB_OP(0x09, rrc_r, REG_C, 0, 2, 8);
    CB_OP(0x0A, rrc_r, REG_D, 0, 2, 8);
    CB_OP(0x0B, rrc_r, REG_E, 0, 2, 8);
    CB_OP(0x0C, rrc_r, REG_H, 0, 2, 8);
    CB_OP(0x0D, rrc_r, REG_L, 0, 2, 8);
    CB_OP(0x0E, rrc_r, REG_HL, 0, 2, 16);
    CB_OP(0x0F, rrc_r, REG_A, 0, 2, 8);

    //RL
    CB_OP(0x10, rl_r, REG_B, 0, 2, 8);
    CB_OP(0x11, rl_r, REG_C, 0, 2, 8);
    CB_OP(0x12, rl_r, REG_D, 0, 2, 8);
    CB_OP(0x13, rl_r, REG_E, 0, 2, 8);
    CB_OP(0x14, rl_r, REG_H, 0, 2, 8);
    CB_OP(0x15, rl_r, REG_L, 0, 2, 8);
    CB_OP(0x16, rl_r, REG_HL, 0, 2, 16);
    CB_OP(0x17, rl_r, REG_A, 0, 2, 8);

    //RR
    CB_OP(0x18, rr_r, REG_B, 0, 2, 8);
    CB_OP(0x19, rr_r, REG_C, 0, 2, 8);
    CB_OP(0x1A, rr_r, REG_D, 0, 2, 8);
    CB_OP(0x1B, rr_r, REG_E, 0, 2, 8);
    CB_OP(0x1C, rr_r, REG_H, 0, 2, 8);
    CB_OP(0x1D, rr_r, REG_L, 0, 2, 8);
    CB_OP(0x1E, rr_r, REG_HL, 0, 2, 16);
    CB_OP(0x1F, rr_r, REG_A, 0, 2, 8);

    //SLA
    CB_OP(0x20, sla_r, REG_B, 0, 2, 8);
    CB_OP(0x21, sla_r, REG_C, 0, 2, 8);
    CB_OP(0x22, sla_r, REG_D, 0, 2, 8);
    CB_OP(0x23, sla_r, REG_E, 0, 2, 8);
    CB_OP(0x24, sla_r, REG_H, 0, 2, 8);
    CB_OP(0x25, sla_r, REG_L, 0, 2, 8);
    CB_OP(0x26, sla_r, REG_HL, 0, 2, 16);
    CB_OP(0x27, sla_r, REG_A, 0, 2, 8);

    //SRA
    CB_OP(0x28, sra_r, REG_B, 0, 2, 8);
    CB_OP(0x29, sra_r, REG_C, 0, 2, 8);
    CB_OP(0x2A, sra_r, REG_D, 0, 2, 8);
    CB_OP(0x2B, sra_r, REG_E, 0, 2, 8);
    CB_OP(0x2C, sra_r, REG_H, 0, 2, 8);
    CB_OP(0x2D, sra_r, REG_L, 0, 2, 8);
    CB_OP(0x2E, sra_r, REG_HL, 0, 2, 16);
    CB_OP(0x2F, sra_r, REG_A, 0, 2, 8);

    //Swap
    CB_OP(0x30, swap_r, REG_B, 0, 2, 8);
    CB_OP(0x31, swap_r, REG_C, 0, 2, 8);
    CB_OP(0x32, swap_r, REG_D, 0, 2, 8);
    CB_OP(0x33, swap_r, REG_E, 0, 2, 8);
    CB_OP(0x34, swap_r, REG_H, 0, 2, 8);
    CB_OP(0x35, swap_r, REG_L, 0, 2, 8);
    CB_OP(0x36, swap_r, REG_HL, 0, 2, 16);
    CB_OP(0x37, swap_r, REG_A, 0, 2, 8);

    //SRL
    CB_OP(0x38, srl_r, REG_B, 0, 2, 8);
    CB_OP(0x39, srl_r, REG_C, 0, 2, 8);
    CB_OP(0x3A, srl_r, REG_D, 0, 2, 8);
    CB_OP(0x3B, srl_r, REG_E, 0, 2, 8);
    CB_OP(0x3C, srl_r, REG_H, 0, 2, 8);
    CB_OP(0x3D, srl_r, REG_L, 0, 2, 8);
    CB_OP(0x3E, srl_r, REG_HL, 0, 2, 16);
    CB_OP(0x3F, srl_r, REG_A, 0, 2, 8);

    //Bit 0
    CB_OP(0x40, bit_b_r, 0, REG_B, 2, 8);
    CB_OP(0x41, bit_b_r, 0, REG_C, 2, 8);
    CB_OP(0x42, bit_b_r, 0, REG_D, 2, 8);
    CB_OP(0x43, bit_b_r, 0, REG_E, 2, 8);
    CB_OP(0x44, bit_b_r, 0, REG_H, 2, 8);
    CB_OP(0x45, bit_b_r, 0, REG_L, 2, 8);
    CB_OP(0x46, bit_b_r, 0, REG_HL, 2, 12);
    CB_OP(0x47, bit_b_r, 0, REG_A, 2, 8);

    //Bit 1
    CB_OP(0x48, bit_b_r, 1, REG_B, 2, 8);
    CB_OP(0x49, bit_b_r, 1, REG_C, 2, 8);
    CB_OP(0x4A, bit_b_r, 1, REG_D, 2, 8);
    CB_OP(0x4B, bit_b_r, 1, REG_E, 2, 8);
    CB_OP(0x4C, bit_b_r, 1, REG_H, 2, 8);
    CB_OP(0x4D, bit_b_r, 1, REG_L, 2, 8);
    CB_OP(0x4E, bit_b_r, 1, REG_HL, 2, 12);
    CB_OP(0x4F, bit_b_r, 1, REG_A, 2, 8);

    //Bit 2
    CB_OP(0x50, bit_b_r, 2, REG_B, 2, 8);
    CB_OP(0x51, bit_b_r, 2, REG_C, 2, 8);
    CB_OP(0x52, bit_b_r, 2, REG_D, 2, 8);
    CB_OP(0x53, bit_b_r, 2, REG_E, 2, 8);
    CB_OP(0x54, bit_b_r, 2, REG_H, 2, 8);
    CB_OP(0x55, bit_b_r, 2, REG_L, 2, 8);
    CB_OP(0x56, bit_b_r, 2, REG_HL, 2, 12);
    CB_OP(0x57, bit_b_r, 2, REG_A, 2, 8);

    //Bit 3
    CB_OP(0x58, bit_b_r, 3, REG_B, 2, 8);
    CB_OP(0x59, bit_b_r, 3, REG_C, 2, 8);
    CB_OP(0x5A, bit_b_r, 3, REG_D, 2, 8);
    CB_OP(0x5B, bit_b_r, 3, REG_E, 2, 8);
    CB_OP(0x5C, bit_b_r, 3, REG_H, 2, 8);
    CB_OP(0x5D, bit_b_r, 3, REG_L, 2, 8);
    CB_OP(0x5E, bit_b_r, 3, REG_HL, 2, 12);
    CB_OP(0x5F, bit_b_r, 3, REG_A, 2, 8);

    //Bit 4
    CB_OP(0x60, bit_b_r, 4, REG_B, 2, 8);
    CB_OP(0x61, bit_b_r, 4, REG_C, 2, 8);
    CB_OP(0x62, bit_b_r, 4, REG_D, 2, 8);
    CB_OP(0x63, bit_b_r, 4, REG_E, 2, 8);
    CB_OP(0x64, bit_b_r, 4, REG_H, 2, 8);
    CB_OP(0x65, bit_b_r, 4, REG_L, 2, 8);
    CB_OP(0x66, bit_b_r, 4, REG_HL, 2, 12);
    CB_OP(0x67, bit_b_r, 4, REG_A, 2, 8);

    //Bit 5
    CB_OP(0x68, bit_b_r, 5, REG_B, 2, 8);
    CB_OP(0x69, bit_b_r, 5, REG_C, 2, 8);
    CB_OP(0x6A, bit_b_r, 5, REG_D, 2, 8);
    CB_OP(0x6B, bit_b_r, 5, REG_E, 2, 8);
    CB_OP(0x6C, bit_b_r, 5, REG_H, 2, 8);
    CB_OP(0x6D, bit_b_r, 5, REG_L, 2, 8);
    CB_OP(0x6E, bit_b_r, 5, REG_HL, 2, 12);
    CB_OP(0x6F, bit_b_r, 5, REG_A, 2, 8);

    //Bit 6
    CB_OP(0x70, bit_b_r, 6, REG_B, 2, 8);
    CB_OP(0x71, bit_b_r, 6, REG_C, 2, 8);
    CB_OP(0x72, bit_b_r, 6, REG_D, 2, 8);
    CB_OP(0x73, bit_b_r, 6, REG_E, 2, 8);
    CB_OP(0x74, bit_b_r, 6, REG_H, 2, 8);
    CB_OP(0x75, bit_b_r, 6, REG_L, 2, 8);
    CB_OP(0x76, bit_b_r, 6, REG_HL, 2, 12);
    CB_OP(0x77, bit_b_r, 6, REG_A, 2, 8);

    //Bit 7
    CB_OP(0x78, bit_b_r, 7, REG_B, 2, 8);
    CB_OP(0x79, bit_b_r, 7, REG_C, 2, 8);
    CB_OP(0x7A, bit_b_r, 7, REG_D, 2, 8);
    CB_OP(0x7B, bit_b_r, 7, REG_E, 2, 8);
    CB_OP(0x7C, bit_b_r, 7, REG_H, 2, 8);
    CB_OP(0x7D, bit_b_r, 7, REG_L, 2, 8);
    CB_OP(0x7E, bit_b_r, 7, REG_HL, 2, 12);
    CB_OP(0x7F, bit_b_r, 7, REG_A, 2, 8);

    //res 0
    CB_OP(0x80, res_b_r, 0, REG_B, 2, 8);
    CB_OP(0x81, res_b_r, 0, REG_C, 2, 8);
    CB_OP(0x82, res_b_r, 0, REG_D, 2, 8);
    CB_OP(0x83, res_b_r, 0, REG_E, 2, 8);
    CB_OP(0x84, res_b_r, 0, REG_H, 2, 8);
    CB_OP(0x85, res_b_r, 0, REG_L, 2, 8);
    CB_OP(0x86, res_b_r, 0, REG_HL, 2, 16);
    CB_OP(0x87, res_b_r, 0, REG_A, 2, 8);

    //res 1
    CB_OP(0x88, res_b_r, 1, REG_B, 2, 8);
    CB_OP(0x89, res_b_r, 1, REG_C, 2, 8);
    CB_OP(0x8A, res_b_r, 1, REG_D, 2, 8);
    CB_OP(0x8B, res_b_r, 1, REG_E, 2, 8);
    CB_OP(0x8C, res_b_r, 1, REG_H, 2, 8);
    CB_OP(0x8D, res_b_r, 1, REG_L, 2, 8);
    CB_OP(0x8E, res_b_r, 1, REG_HL, 2, 16);
    CB_OP(0x8F, res_b_r, 1, REG_A, 2, 8);

    //res 2
    CB_OP(0x90, res_b_r, 2, REG_B, 2, 8);
    CB_OP(0x91, res_b_r, 2, REG_C, 2, 8);
    CB_OP(0x92, res_b_r, 2, REG_D, 2, 8);
    CB_OP(0x93, res_b_r, 2, REG_E, 2, 8);
    CB_OP(0x94, res_b_r, 2, REG_H, 2, 8);
    CB_OP(0x95, res_b_r, 2, REG_L, 2, 8);
    CB_OP(0x96, res_b_r, 2, REG_HL, 2, 16);
    CB_OP(0x97, res_b_r, 2, REG_A, 2, 8);

    //res 3
    CB_OP(0x98, res_b_r, 3, REG_B, 2, 8);
    CB_OP(0x99, res_b_r, 3, REG_C, 2, 8);
    CB_OP(0x9A, res_b_r, 3, REG_D, 2, 8);
    CB_OP(0x9B, res_b_r, 3, REG_E, 2, 8);
    CB_OP(0x9C, res_b_r, 3, REG_H, 2, 8);
    CB_OP(0x9D, res_b_r, 3, REG_L, 2, 8);
    CB_OP(0x9E, res_b_r, 3, REG_HL, 2, 16);
    CB_OP(0x9F, res_b_r, 3, REG_A, 2, 8);

    //res 4
    CB_OP(0xA0, res_b_r, 4, REG_B, 2, 8);
    CB_OP(0xA1, res_b_r, 4, REG_C, 2, 8);
    CB_OP(0xA2, res_b_r, 4, REG_D, 2, 8);
    CB_OP(0xA3, res_b_r, 4, REG_E, 2, 8);
    CB_OP(0xA4, res_b_r, 4, REG_H, 2, 8);
    CB_OP(0xA5, res_b_r, 4, REG_L, 2, 8);
    CB_OP(0xA6, res_b_r, 4, REG_HL, 2, 16);
    CB_OP(0xA7, res_b_r, 4, REG_A, 2, 8);

    //res 5
    CB_OP(0xA8, res_b_r, 5, REG_B, 2, 8);
    CB_OP(0xA9, res_b_r, 5, REG_C, 2, 8);
    CB_OP(0xAA, res_b_r, 5, REG_D, 2, 8);
    CB_OP(0xAB, res_b_r, 5, REG_E, 2, 8);
    CB_OP(0xAC, res_b_r, 5, REG_H, 2, 8);
    CB_OP(0xAD, res_b_r, 5, REG_L, 2, 8);
    CB_OP(0xAE, res_b_r, 5, REG_HL, 2, 16);
    CB_OP(0xAF, res_b_r, 5, REG_A, 2, 8);

    //res 6
    CB_OP(0xB0, res_b_r, 6, REG_B, 2, 8);
    CB_OP(0xB1, res_b_r, 6, REG_C, 2, 8);
    CB_OP(0xB2, res_b_r, 6, REG_D, 2, 8);
    CB_OP(0xB3, res_b_r, 6, REG_E, 2, 8);
    CB_OP(0xB4, res_b_r, 6, REG_H, 2, 8);
    CB_OP(0xB5, res_b_r, 6, REG_L, 2, 8);
    CB_OP(0xB6, res_b_r, 6, REG_HL, 2, 16);
    CB_OP(0xB7, res_b_r, 6, REG_A, 2, 8);

    //res 7
    CB_OP(0xB8, res_b_r, 7, REG_B, 2, 8);
    CB_OP(0xB9, res_b_r, 7, REG_C, 2, 8);
    CB_OP(0xBA, res_b_r, 7, REG_D, 2, 8);
    CB_OP(0xBB, res_b_r, 7, REG_E, 2, 8);
    CB_OP(0xBC, res_b_r, 7, REG_H, 2, 8);
    CB_OP(0xBD, res_b_r, 7, REG_L, 2, 8);
    CB_OP(0xBE, res_b_r, 7, REG_HL, 2, 16);
    CB_OP(0xBF, res_b_r, 7, REG_A, 2, 8);

    //set 0
    CB_OP(0xC0, set_b_r, 0, REG_B, 2, 8);
    CB_OP(0xC1, set_b_r, 0, REG_C, 2, 8);
    CB_OP(0xC2, set_b_r, 0, REG_D, 2, 8);
    CB_OP(0xC3, set_b_r, 0, REG_E, 2, 8);
    CB_OP(0xC4, set_b_r, 0, REG_H, 2, 8);
    CB_OP(0xC5, set_b_r, 0, REG_L, 2, 8);
    CB_OP(0xC6, set_b_r, 0, REG_HL, 2, 16);
    CB_OP(0xC7, set_b_r, 0, REG_A, 2, 8);

    //set 1
    CB_OP(0xC8, set_b_r, 1, REG_B, 2, 8);
    CB_OP(0xC9, set_b_r, 1, REG_C, 2, 8);
    CB_OP(0xCA, set_b_r, 1, REG_D, 2, 8);
    CB_OP(0xCB, set_b_r, 1, REG_E, 2, 8);
    CB_OP(0xCC, set_b_r, 1, REG_H, 2, 8);
    CB_OP(0xCD, set_b_r, 1, REG_L, 2, 8);
    CB_OP(0xCE, set_b_r, 1, REG_HL, 2, 16);
    CB_OP(0xCF, set_b_r, 1, REG_A, 2, 8);

    //set 2
    CB_OP(0xD0, set_b_r, 2, REG_B, 2, 8);
    CB_OP(0xD1, set_b_r, 2, REG_C, 2, 8);
    CB_OP(0xD2, set_b_r, 2, REG_D, 2, 8);
    CB_OP(0xD3, set_b_r, 2, REG_E, 2, 8);
    CB_OP(0xD4, set_b_r, 2, REG_H, 2, 8);
    CB_OP(0xD5, set_b_r, 2, REG_L, 2, 8);
    CB_OP(0xD6, set_b_r, 2, REG_HL, 2, 16);
    CB_OP(0xD7, set_b_r, 2, REG_A, 2, 8);

    //set 3
    CB_OP(0xD8, set_b_r, 3, REG_B, 2, 8);
    CB_OP(0xD9, set_b_r, 3, REG_C, 2, 8);
    CB_OP(0xDA, set_b_r, 3, REG_D, 2, 8);
    CB_OP(0xDB, set_b_r, 3, REG_E, 2, 8);
    CB_OP(0xDC, set_b_r, 3, REG_H, 2, 8);
    CB_OP(0xDD, set_b_r, 3, REG_L, 2, 8);
    CB_OP(0xDE, set_b_r, 3, REG_HL, 2, 16);
    CB_OP(0xDF, set_b_r, 3, REG_A, 2, 8);

    //set 4
    CB_OP(0xE0, set_b_r, 4, REG_B, 2, 8);
    CB_OP(0xE1, set_b_r, 4, REG_C, 2, 8);
    CB_OP(0xE2, set_b_r, 4, REG_D, 2, 8);
    CB_OP(0xE3, set_b_r, 4, REG_E, 2, 8);
    CB_OP(0xE4, set_b_r, 4, REG_H, 2, 8);
    CB_OP(0xE5, set_b_r, 4, REG_L, 2, 8);
    CB_OP(0xE6, set_b_r, 4, REG_HL, 2, 16);
    CB_OP(0xE7, set_b_r, 4, REG_A, 2, 8);

    //set 5
    CB_OP(0xE8, set_b_r, 5, REG_B, 2, 8);
    CB_OP(0xE9, set_b_r, 5, REG_C, 2, 8);
    CB_OP(0xEA, set_b_r, 5, REG_D, 2, 8);
    CB_OP(0xEB, set_b_r, 5, REG_E, 2, 8);
    CB_OP(0xEC, set_b_r, 5, REG_H, 2, 8);
    CB_OP(0xED, set_b_r, 5, REG_L, 2, 8);
    CB_OP(0xEE, set_b_r, 5, REG_HL, 2, 16);
    CB_OP(0xEF, set_b_r, 5, REG_A, 2, 8);

    //set 6
    CB_OP(0xF0, set_b_r, 6, REG_B, 2, 8);
    CB_OP(0xF1, set_b_r, 6, REG_C, 2, 8);
    CB_OP(0xF2, set_b_r, 6, REG_D, 2, 8);
    CB_OP(0xF3, set_b_r, 6, REG_E, 2, 8);
    CB_OP(0xF4, set_b_r, 6, REG_H, 2, 8);
    CB_OP(0xF5, set_b_r, 6, REG_L, 2, 8);
    CB_OP(0xF6, set_b_r, 6, REG_HL, 2, 16);
    CB_OP(0xF7, set_b_r, 6, REG_A, 2, 8);

    //set 7
    CB_OP(0xF8, set_b_r, 7, REG_B, 2, 8);
    CB_OP(0xF9, set_b_r, 7, REG_C, 2, 8);
    CB_OP(0xFA, set_b_r, 7, REG_D, 2, 8);
    CB_OP(0xFB, set_b_r, 7, REG_E, 2, 8);
    CB_OP(0xFC, set_b_r, 7, REG_H, 2, 8);
    CB_OP(0xFD, set_b_r, 7, REG_L, 2, 8);
    CB_OP(0xFE, set_b_r, 7, REG_HL, 2, 16);
    CB_OP(0xFF, set_b_r, 7, REG_A, 2, 8);
}