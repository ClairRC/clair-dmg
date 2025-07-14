#ifndef PPU_STATE_H
#define PPU_STATE_H

#include <stdint.h>
#include "hardware_def.h"

typedef struct {
	uint8_t lcd_on; //Whether LCD is on or not TODO: Might be redundant?
	uint32_t frame_time; //Current frame time
	PPU_Mode current_mode;
} GlobalPPUState;

#endif