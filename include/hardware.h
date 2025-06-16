#ifndef HARDWARE_H 
#define HARDWARE_H 

#include "system.h"

//Responsible for updating the state of the hardware after instructions get called.
//I'm not sure if this is how I should organize this, but for now I'll keep all of the
//update functions centralized in here.

void sync_hardware(EmulatorSystem*, uint16_t); //Updates system hardware given ticks passed
void update_timer_registers(EmulatorSystem*);
void update_dma_transfer(EmulatorSystem*);
void update_stat_register(EmulatorSystem*); 

#endif 
