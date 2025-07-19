#ifndef SYSTEM_STATE_H
#define SYSTEM_STATE_H

#include "ppu_state.h"
#include "apu_state.h"
#include "dma_state.h"
#include "timer_state.h"

typedef struct {
	GlobalPPUState* ppu_state;
	GlobalAPUState* apu_state;
	GlobalDMAState* dma_state;
	GlobalTimerState* timer_state;

	uint8_t running; //Whether or not the emulator system is currently running or not
} GlobalSystemState;

GlobalSystemState* system_state_init();
void system_state_destroy(GlobalSystemState* state);

#endif