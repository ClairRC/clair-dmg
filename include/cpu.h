#ifndef CPU_H
#define CPU_H
#include <stdint.h>
#include "memory_bus.h"

//Register file struct
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

//Struct for some special CPU flags
//CPU owns its own state because nothing else needs to know about it
typedef struct {
    int IME; //Master interrupt enable
    int enableIME; //EI instruction is delayed by 1 instruction, so this tracks that state
    int isHalted; //Checks if CPU is currently in HALT mode
    int halt_bug; //Bug that skips first byte after halt is exited based on interrupt flags
} LocalCPUState;

typedef struct {
    RegisterFile registers;
    LocalCPUState state;
    MemoryBus* bus;
} CPU;

CPU* cpu_init(MemoryBus*);
void cpu_destroy(CPU*);
uint16_t getRegisterValue16(CPU*, Registers);
uint8_t getRegisterValue8(CPU*, Registers);
int setRegisterValue(CPU*, Registers, uint16_t);
int setFlag(CPU*, Flags);
int clearFlag(CPU*, Flags);
int updateFlag(CPU*, Flags, int);
int flagIsSet(CPU*, Flags);

#endif