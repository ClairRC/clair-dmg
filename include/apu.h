#ifndef APU_H
#define APU_H

#include <stdint.h>
#include "memory_bus.h"
#include "sdl_data.h"
#include "apu_state.h"

//Channel state structs
typedef struct {
	//Channel 1 specific values
	uint8_t enable;
	uint8_t dac_enable;

	uint64_t emulator_time_start;

	//Length timer values
	uint8_t length_timer; //Ticks up from specified value with DIV-APU until it reaches 64, then turns off the channel
	uint8_t length_timer_end; //Ch1 ends at 64

	//Period timer values
	uint16_t period_div; //11-bit divider that samples the waveform when it overflows. Ticks every 4 dots
	uint16_t period_start; //Value that period div gets set to when it overflows

	//Envelope values
	uint8_t high_vol; //Volume of "high" part of pulse wave
	uint8_t env_dir; //Whether volume increases or decreases
	uint8_t env_timer; //Starts at 0. Every time this reaches env_end, it updates the "high_vol" variable. Ticks ever 64Hz
	uint8_t env_end; //End of timer

	//Frequency sweep values
	uint8_t sweep_enable;
	uint8_t sweep_timer; 
	uint8_t sweep_end; //How often sweep iterations happen
	uint8_t sweep_dir; //Which direction frequency changes
	uint8_t sweep_step; //How much it changes by

	//Current sample of duty cycle
	uint8_t sample_num;

	uint8_t out;
} Ch1State;

typedef struct {
	//Channel 2 spceific values
	uint8_t enable;
	uint8_t dac_enable;

	uint64_t emulator_time_start;

	//Length timer values
	uint8_t length_timer; //Ticks up from specified value with DIV-APU until it reaches 64, then turns off the channel
	uint8_t length_timer_end; //Ch2 ends at 64

	//Period timer values
	uint16_t period_div; //11-bit divider that samples the waveform when it overflows. Ticks every 4 dots
	uint16_t period_start; //Value that period div gets set to when it overflows

	//Envelope values
	uint8_t high_vol; //Volume of "high" part of pulse wave
	uint8_t env_dir; //Whether volume increases or decreases
	uint8_t env_timer; //Starts at 0. Every time this reaches env_end, it updates the "high_vol" variable. Ticks ever 64Hz
	uint8_t env_end; //End of timer

	//Current sample of duty cycle
	uint8_t sample_num;

	uint8_t out;
}Ch2State;

typedef struct {
	//Channel 3 specific values 
	uint8_t enable;
	uint8_t dac_enable;

	uint64_t emulator_time_start;

	//Length timer values
	uint16_t length_timer; //Ticks up from specified value with DIV-APU until it reaches 256, then turns off the channel
	uint16_t length_timer_end; //Ch3 ends at 256

	//Period timer values
	uint16_t period_div; //11-bit divider that samples the waveform when it overflows. Ticks every 4 dots
	uint16_t period_start; //Value that period div gets set to when it overflows

	//Wave RAM index
	uint8_t sample_num; //Which out of 32 samples are being read from custom waveform in wave RAM

	//Ch3 output
	uint8_t out;
}Ch3State;

typedef struct {
	//Channel 4 specific values 
	uint8_t enable;
	uint8_t dac_enable;

	uint64_t emulator_time_start;

	uint8_t out;
}Ch4State;

//Local state variables for APU
typedef struct {
	uint8_t div_bit; //Which bit of DIV to check. On DMG this is always 4, but depending on double speed mode this can be bit 5
	uint8_t prev_div_state; //Previous DIV bit to detect updates for incrementing APU DIV
	uint8_t apu_div; //APU div is connected to div and ticks up at 256Hz given no writes to DIV
	uint8_t apu_div_updated; //Flag for whether or not an update occured and certain things need to be checked

	//Individual channel state
	Ch1State ch1;
	Ch2State ch2;
	Ch3State ch3;
	Ch4State ch4;
} LocalAPUState;

typedef struct {
	//Memory bus pointer
	MemoryBus* bus;

	//SDL Stuff
	SDL_Audio_Data* sdl_data;

	//APU Duty cycles
	uint8_t duty_cycles[32]; //4 options with 8 samples each. This determines how much of a pulse wave is high vs low

	//Local state
	LocalAPUState local_state;

	//Global state
	GlobalAPUState* global_state;
} APU;

//APU initialization and destruction
APU* apu_init(MemoryBus* bus, GlobalAPUState* global_state, SDL_Audio_Data* sdl_data);
void apu_destroy(APU* apu);

void fill_buffer(APU* apu);
int16_t mix_dac_values(APU* apu); //Gets the mixed DAC value to add to audio buffer

void update_apu(APU* apu, uint64_t emulator_time);
void update_dacs(APU* apu);
void update_div_apu(APU* apu);
void update_channel_active(APU* apu, uint64_t emulator_time);

void turn_off_apu(APU* apu);

void enable_channel_1(APU* apu, uint64_t emulator_time); //Enables channel 1
void enable_channel_2(APU* apu, uint64_t emulator_time); //Enables channel 2
void enable_channel_3(APU* apu, uint64_t emulator_time); //Enables channel 3
void enable_channel_4(APU* apu, uint64_t emulator_time); //Enables channel 4

void update_ch1(APU* apu, uint64_t emulator_time);
void update_ch2(APU* apu, uint64_t emulator_time);
void update_ch3(APU* apu, uint64_t emulator_time);
void update_ch4(APU* apu, uint64_t emulator_time);

#endif