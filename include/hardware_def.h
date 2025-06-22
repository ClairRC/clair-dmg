#ifndef HARDWARE_DEF_H
#define HARDWARE_DEF_H
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

//Enum so memory struct knows who is accessing it
//This is because during certain modes of the PPU or DMA transfer,
//some memory reads are blocked from the CPU
typedef enum {
    CPU_ACCESS,
    PPU_ACCESS,
    DMA_ACCESS
} Accessor;

//Enum for PPU modes. This is used by the memory module to
//restrict CPU access during certain modes. It also allows the PPU
//to set the flag for when the mode changes
typedef enum {
    PPU_MODE_0 = 0,
    PPU_MODE_1 = 1,
    PPU_MODE_2 = 2,
    PPU_MODE_3 = 3,
    PPU_MODE_OFF
} PPU_Mode;

//Specifices the specific base MBC (Memory Banking Control) type
//TODO: Implement the rest of these
typedef enum {
    MBC_NONE,
    MBC_1,
    MBC_2,
    MBC_3,
    MBC_5
} MBC_Type;

#endif