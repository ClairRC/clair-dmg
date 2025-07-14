#ifndef TIMER_STATE_H
#define TIMER_STATE_H

#include <stdint.h>

//This holds state data for the system timer and also the timer registers, as they are connected

typedef struct {
	uint64_t elapsed_time; //Elapsed time the emulator has been running. This never gets reset
	uint16_t system_time; //System timer (~4MHz)
} GlobalTimerState;

#endif