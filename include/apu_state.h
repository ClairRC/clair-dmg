#ifndef APU_STATE_H
#define APU_STATE_H

typedef struct {
	uint8_t turn_off_apu; //Whether or not APU should be turned off 
	uint8_t apu_enable; //Wether or not APU is current enabled

	uint8_t trigger_ch1; //Trigger for channels
	uint8_t trigger_ch2;
	uint8_t trigger_ch3;
	uint8_t trigger_ch4;

	uint8_t ch1_length_enable; //Length enable flags get updated when memory is written to
	uint8_t ch2_length_enable; 
	uint8_t ch3_length_enable; 
	uint8_t ch4_length_enable;
} GlobalAPUState;

#endif