#ifndef APU_H
#define APU_H

#include <stdint.h>
#include "memory_bus.h"
#include "sdl_data.h"

//Gameboy's audio processing unit, which makes the sounds you hear

//Enums for the different channels
typedef enum {
	CHANNEL_1 = 0,
	CHANNEL_2 = 1,
	CHANNEL_3 = 2,
	CHANNEL_4 = 3
}ChannelNumber;

//Structs for each channel
typedef struct {
	//Channel 1
	uint8_t apu_div_start_time;
	uint16_t emulator_start_time;

	uint8_t active;
	uint8_t length_enable;
	uint8_t sweep_enable;
	uint8_t env_enable;

	uint16_t period_div;
	uint16_t period_begin;

	uint8_t length_timer;
	
	uint8_t env_timer;
	uint8_t env_overflow;
	uint8_t env_direction; //0 means decrease, 1 means increase

	uint8_t sweep_timer;
	uint8_t sweep_overflow;
	uint8_t sweep_step;
	uint8_t sweep_direction; //0 means period increases, 1 means decrease

	uint8_t current_sample;
	uint8_t total_samples;

	uint8_t wave_volume; //Volume of "peak" of duty cycle

	uint8_t out; //Volume being played
}Channel1;

typedef struct {
	//Channel 2
	uint8_t apu_div_start_time;
	uint16_t emulator_start_time;

	uint8_t active;
	uint8_t length_enable;
	uint8_t sweep_enable;
	uint8_t env_enable;

	uint16_t period_div;
	uint16_t period_begin;

	uint8_t length_timer;

	uint8_t env_timer;
	uint8_t env_overflow;
	uint8_t env_direction; //0 means decrease, 1 means increase

	uint8_t sweep_timer;
	uint8_t sweep_overflow;
	uint8_t sweep_step;

	uint8_t current_sample;
	uint8_t total_samples;

	uint8_t wave_volume; //Volume of "peak" of duty cycle

	uint8_t out;
}Channel2;

typedef struct {
	//Channel 3
	uint8_t apu_div_start_time;
	uint16_t emulator_start_time;

	uint8_t active;
	uint8_t length_enable;

	uint16_t length_timer;

	uint16_t period_begin;
	uint16_t period_div;

	uint8_t current_sample;
	uint8_t total_samples;

	uint8_t out;
}Channel3;

typedef struct {
	//Channel 4
	uint8_t apu_div_start_time;
	uint16_t emulator_start_time;

	uint8_t active;
	uint8_t length_enable;
	uint8_t env_enable;

	uint8_t length_timer;

	uint16_t period_begin;
	uint16_t period_div;

	uint8_t env_timer;
	uint8_t env_overflow;
	uint8_t env_direction; //0 means decrease, 1 means increase

	uint16_t lfsr;

	uint8_t wave_volume; //Volume of "peak" of duty cycle

	uint8_t out;
}Channel4;

//The APU is comprised primarily of a bunch of timers
typedef struct {
	//Memory pointer
	MemoryBus* bus;

	//Timers
	uint8_t prev_div_bit;
	uint8_t div_apu;
	uint8_t prev_div_apu;

	//Duty cycle values
	//4 options with 8 samples each
	//0 means low, 1 means high
	uint8_t duty_cycles[32];

	//Channels
	Channel1 ch1;
	Channel2 ch2;
	Channel3 ch3;
	Channel4 ch4;

} APU;

APU* apu_init(MemoryBus* bus);
void apu_destroy(APU* apu);

//Adds sample to audio buffer
void add_sample(APU* apu);

//Mixes all current DAC values to s8int and returns it for SDL to sample
int8_t mix_output(APU* apu);

void activate_ch1(APU* apu, uint16_t emulator_time);
void activate_ch2(APU* apu, uint16_t emulator_time);
void activate_ch3(APU* apu, uint16_t emulator_time);
void activate_ch4(APU* apu, uint16_t emulator_time);

void deactivate_channel(APU* apu, ChannelNumber channel); //Deactivates given channel

void update_apu(APU* apu, uint16_t emulator_time);
void update_ch1(APU* apu, uint16_t emulator_time);
void update_ch2(APU* apu, uint16_t emulator_time);
void update_ch3(APU* apu, uint16_t emulator_time);
void update_ch4(APU* apu, uint16_t emulator_time);

void update_volume_envelope(APU* apu, ChannelNumber channel);
int update_ch1_freq_sweep(APU* apu);

uint8_t get_duty_cycle(APU* apu, ChannelNumber channel);

#endif