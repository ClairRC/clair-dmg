#ifndef CPU_H
#define CPU_H
#include <stdint.h>

//CPU Flags (stored in F register)
//ie. FLAG_ZERO = 1000 0000 because most significant digit is zero flag
#define FLAG_ZERO (1<<7)
#define FLAG_SUB (1<<6)
#define FLAG_HALFCARRY (1<<5)
#define FLAG_CARRY (1<<4)

//Register enums to make sorting through all the
//different opcodes a LITTLE less annoying
typedef enum {
    REG_A,
    REG_B,
    REG_C,
    REG_D,
    REG_E,
    REG_F,
    REG_H,
    REG_L,
    REG_sp,
    REG_pc,
    REG_AF,
    REG_BC,
    REG_DE,
    REG_HL
} Registers;

//Flag enums to help with setting flags
typedef enum {
    ZERO,
    SUB,
    HALFCARRY,
    CARRY
} Flags;

/*CPU architecture and related functions*/
typedef struct {
    uint8_t A;
    uint8_t B;
    uint8_t C;
    uint8_t D;
    uint8_t E;
    uint8_t F;
    uint8_t H;
    uint8_t L;
    uint16_t sp;
    uint16_t pc;
} RegisterFile;

typedef struct {
    RegisterFile registers;
} CPU;

CPU* cpu_init(void);
void cpu_destroy(CPU* cpu);
uint16_t getRegisterValue16(CPU* cpu, Registers reg);
uint8_t getRegisterValue8(CPU* cpu, Registers reg);
int setRegisterValue(CPU* cpu, Registers reg, uint16_t value);
int setFlag(CPU* cpu, Flags flag);
int clearFlag(CPU* cpu, Flags flag);
int flagIsSet(CPU* cpu, Flags flag);

#endif