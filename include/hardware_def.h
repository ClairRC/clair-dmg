//Defines enums that are used throughout the emulator

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
    //Flags stored in F register
    ZERO,
    SUB,
    HALFCARRY,
    CARRY,

    //Special CPU state flags
    IME,
    ENABLE_IME,
    IS_HALTED,
    HALT_BUG
} Flags;
