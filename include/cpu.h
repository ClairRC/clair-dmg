#ifndef CPU_H
#define CPU_H
#include <stdint.h>
#include "hardware_def.h"
#include "memory.h"

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

//CPU struct
/*
* TODO:
* Add T-Cycle and M-Cycle implementation
* Add and initialize "memory bus" (pointer to memory)
*/

typedef struct {
    RegisterFile registers;
} CPU;

CPU* cpu_init(void);
void cpu_destroy(CPU*);
uint16_t getRegisterValue16(CPU*, Registers);
uint8_t getRegisterValue8(CPU*, Registers);
int setRegisterValue(CPU*, Registers, uint16_t);
int setFlag(CPU*, Flags);
int clearFlag(CPU*, Flags);
int updateFlag(CPU*, Flags, int);
int flagIsSet(CPU*, Flags);

#endif