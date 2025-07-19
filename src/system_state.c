#include "system_state.h"
#include "logging.h"
#include <stdlib.h>


GlobalSystemState* system_state_init() {
	GlobalSystemState* system_state = (GlobalSystemState*)malloc(sizeof(GlobalSystemState));
	GlobalPPUState* ppu_state = (GlobalPPUState*)malloc(sizeof(GlobalPPUState));
	GlobalDMAState* dma_state = (GlobalDMAState*)malloc(sizeof(GlobalDMAState));
	GlobalTimerState* timer_state = (GlobalTimerState*)malloc(sizeof(GlobalTimerState));
	GlobalAPUState* apu_state = (GlobalAPUState*)calloc(1, sizeof(GlobalAPUState));

	if (system_state == NULL || ppu_state == NULL || dma_state == NULL || timer_state == NULL || apu_state == NULL) {
		printError("Error initializing system state");
		system_state_destroy(system_state);
		return NULL;
	}

	ppu_state->current_mode = PPU_MODE_2;
	ppu_state->frame_time = 0;
	ppu_state->lcd_on = 1;

	dma_state->active = 0;
	dma_state->remaining_cycles = 640;
	dma_state->source = 0x00;

	timer_state->system_time = 0; //System timer (~4MHz)
	timer_state->elapsed_time = 0; //Elapsed time the emulator has been running in "dots" (single-speed t-cycles) for timing

	system_state->dma_state = dma_state;
	system_state->ppu_state = ppu_state;
	system_state->timer_state = timer_state;
	system_state->apu_state = apu_state;

	system_state->running = 1;

	return system_state;
}

void system_state_destroy(GlobalSystemState* state) {
	if (state == NULL)
		return;

	if (state->ppu_state != NULL) { free(state->ppu_state); }
	if (state->dma_state != NULL) { free(state->dma_state); }
	if (state->timer_state != NULL) { free(state->timer_state); }
	if (state->apu_state != NULL) { free(state->apu_state); }

	free(state);
}

