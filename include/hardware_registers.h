#ifndef HARDWARE_REGISTERS_H
#define HARDWARE_REGISTERS_H
#include <stdint.h> 

//Defines and encapsulates the read/write information
//for the hardware registers in the range 0xFF00 - 0xFFFF

//Struct for information
typedef struct {
    uint8_t read_mask;
    uint8_t write_mask;
    uint8_t cgb_only;
} HardwareRegister;

//Table and populate function
extern HardwareRegister hw_registers[256];
void init_hw_registers(void);

#endif