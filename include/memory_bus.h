#ifndef MEMORY_BUS_H
#define MEMORY_BUS_H

#include <stdint.h>

#include "system_state.h"
#include "memory.h"
#include "hardware_def.h"

//Memory bus struct which handles the states of each subsystem during memory reads/writes

/*
* Memory bus will help be an interface between memory reads/writes and the state of the rest of the system
*/

typedef struct {
	GlobalSystemState* system_state; //Has a reference to the states of systems it needs
	Memory* memory; //Reference to memory, which holds the actual memory values
} MemoryBus;

MemoryBus* memory_bus_init(Memory* mem, GlobalSystemState* system_state);
void memory_bus_destroy(MemoryBus* bus);
uint8_t mem_read(MemoryBus* bus, uint16_t address, Accessor accessor);
uint8_t mem_write(MemoryBus* bus, uint16_t address, uint8_t new_val, Accessor accessor);
void update_global_state(MemoryBus* bus, uint16_t address, uint8_t new_val);
uint8_t get_input_byte(Memory* mem, uint8_t val);
uint8_t mask_hw_reg_read(uint8_t val, uint16_t address);
uint8_t mask_hw_reg_write(uint8_t new_val, uint8_t old_val, uint16_t address);
uint8_t mem_accessible(MemoryBus* bus, MemoryRange range, Accessor accessor);

#endif