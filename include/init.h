#ifndef INIT_H
#define INIT_H

#include "system.h" 

//Sets up initial emulator conditions
int emulator_init();
Memory* load_rom_data();
int init_cpu_vals(EmulatorSystem*);

#endif
