#include "apu.h"
#include "logging.h"

/*
* The GameBoy's APU is what controls the sounds that come frome the speaker.
* This is full of quite a few tricky details and a lot of timing to keep up with.
* It is all connected for the most part to the APU DIV which is directly connected to the DIV register.
* Nintendo accounted for CGB double speed mode by making the APU DIV increment for either bit 4 or 5
* depending on double speed mode. This effectively means that the APU works the same regardless of speed mode.
* 
* CH1 and CH2 are pulse waves, CH3 is a custom waveform, and CH4 produces psuedo-random noise.
* Interestingly, instead of changing the sound by manipulating the frequency of the wave, the Gameboy
* allows you to change the period of the wave, stepping through the different "samples" at different rates, 
* which effectively increases the frequency the faster you go through them, which decreases the period.
*/

//Init APU
APU* apu_init(MemoryBus* bus) {
	APU* apu = (APU*)malloc(sizeof(APU));

	if (apu == NULL) {
		printError("Error intializing APU");
		return NULL;
	}

	if (bus == NULL) {
		printError("Error initializing APU.");
		return NULL;
	}

	apu->bus = bus;
	apu->prev_div_bit = 0;
	apu->div_apu = 0;
	apu->prev_div_apu = 0;

	//All of the channel specific stuff gets set when they're activated, so just make sure they start off
	apu->ch1.active = 0;
	apu->ch2.active = 0;
	apu->ch3.active = 0;
	apu->ch4.active = 0;

	//Duty cycles for pulse wave
	uint8_t duty_cycles[32] = {
		//12.5%
		1, 1, 1, 1, 1, 1, 1, 0,
		//25%
		0, 1, 1, 1, 1, 1, 1, 0,
		//50%
		0, 1, 1, 1, 1, 0, 0, 0,
		//75%
		1, 0, 0, 0, 0, 0, 0, 1
	};
	
	//Copy values over
	memcpy(apu->duty_cycles, duty_cycles, 32 * sizeof(uint8_t));

	return apu;
}

void apu_destroy(APU* apu) {
	if (apu != NULL) {
		free(apu);
	}
}

//Updates all APU data
void update_apu(APU* apu, uint16_t emulator_time) {
	if (apu->ch1.active)
		update_ch1(apu, emulator_time);
	if (apu->ch2.active)
		update_ch2(apu, emulator_time);
	if (apu->ch3.active)
		update_ch3(apu, emulator_time);

	//Every 95 dots, add sample to SDL audio stream
	if (emulator_time % 95 == 0) {
		add_sample(apu);
	}
}

//Adds current mixed channel values to SDL sample...
//Since it gets sampled at 44.1kHz, this shoudl be about ever 95 dots
void add_sample(APU* apu) {
	
}

//Mixes current channel values
int8_t mix_output(APU* apu) {
	//I am not knowledgeable about how SOUND works so I'm
	//just really hoping I'm understanding this well enough

	//If audio is off, return 0
	if (!(apu->bus->memory->NR52_LOCATION & (1 << 7)))
		return 0;

	//Get each channel's output centered at 0
	int16_t ch1_out;
	int16_t ch2_out;
	int16_t ch3_out;
	int16_t ch4_out = 0; //TODO: Implement channel 4
	
	//If DAC is off, value is 0, otherwise take current value and subtract by 8
	//Since each value is between 0 and 15, this will center it at 0
	
	//Ch1
	if (!(apu->bus->memory->NR12_LOCATION & 0xF8))
		ch1_out = 0;
	else
		ch1_out = (int16_t)apu->ch1.out - 8;

	//Ch2
	if (!(apu->bus->memory->NR22_LOCATION & 0xF8))
		ch2_out = 0;
	else
		ch2_out = (int16_t)apu->ch2.out - 8;

	//Ch3
	if (!(apu->bus->memory->NR30_LOCATION & 0x80))
		ch3_out = 0;
	else
		ch3_out = (int16_t)apu->ch3.out - 8;

	//Add all together and scale to 8 bit
	int16_t out = 0;

	out = ch1_out + ch2_out + ch3_out + ch4_out; 
	out = (out * 128) / 32; //Multiply by 128 so max value is 32 * 128 on either side, divide by 32 to scale it

	return (int8_t)out; //Cast before return. Just in case to avoid weird overflows or anything...
}

//Sets initial values for channel 1
void activate_ch1(APU* apu, uint16_t emulator_time) {
	apu->ch1.active = 1;
	apu->ch1.current_sample = 0;
	apu->ch1.apu_div_start_time = apu->div_apu;
	apu->ch1.emulator_start_time = emulator_time;

	uint8_t nr10 = apu->bus->memory->NR10_LOCATION;
	uint8_t nr11 = apu->bus->memory->NR11_LOCATION;
	uint8_t nr12 = apu->bus->memory->NR12_LOCATION;
	uint8_t nr13 = apu->bus->memory->NR13_LOCATION;
	uint8_t nr14 = apu->bus->memory->NR14_LOCATION;

	/*
	* For my own sanity: 
	* Length timer starts at value specified, counts down up every 2 APU DIV ticks. Turns channel off when it reaches 64 
	* 
	* Sweep timer starts 0, counts up every 4 APU DIV ticks, when it reaches sweep_end, 
	* new period is calculated based on sweep_step, and reset timer to 0. If sweep end is 0, sweeps are disabled
	* 
	* Envelope timer starts at 0, counts up every 8 APU DIV ticks, when it reaches envelope_end,
	* new volume is caluclated, reset timer to 0.
	* If envelope end is 0, no envelope is done
	*/

	//If length timer expired, reset it
	if (apu->ch1.length_timer >= 64) {
		//Initial length timer is bottom 5 bits of NR11
		apu->ch1.length_timer = nr11 & (0x1F);
	}

	//Bottom 3 bits of NR14 is upper 3 of period value
	//NR13 is bottom 8 bits of starting period
	apu->ch1.period_begin = ((nr14 & 0x7) << 8) | nr13;
	apu->ch1.period_div = apu->ch1.period_begin;

	//Envelope end tells how high the timer needs to get before adjusting volume
	//Timer starts at 0
	apu->ch1.env_overflow = nr12 & 0x7;
	apu->ch1.env_timer = 0;
	apu->ch1.env_direction = (nr12 >> 3) & 0x1; //Whether volume increases or decreases

	//Sweep step is bottom 3 bits of NR10
	//Sweep pace is bits 4-6
	apu->ch1.sweep_step = nr10 & 0x7;
	apu->ch1.sweep_overflow = (nr10 >> 4) & 0x7;
	apu->ch1.sweep_timer = 0;
	apu->ch1.sweep_direction = (nr10 >> 3) & 0x1;

	//If this is non-zero, immediately check if period overflows
	//I don't see why this ever shoudl be the case, but apparently it checks this, so I'll keep it for now
	if (apu->ch1.sweep_step != 0) {
		if (apu->ch1.period_begin >= 0x800)
			apu->ch1.active = 0;
	}

	//Enable/Disable flags
	apu->ch1.length_enable = nr14 & (1 << 6) ? 1 : 0;
	apu->ch1.sweep_enable = (apu->ch1.sweep_step || apu->ch1.sweep_overflow);
	apu->ch1.env_enable = apu->ch1.env_overflow ? 1 : 0;

	//Initial volume is top 4 bits of NR12
	apu->ch1.wave_volume = (nr12 & 0xF0) >> 4;
	apu->ch1.out = 0; //0 By default
}

//Set initial values for channel 2
void activate_ch2(APU* apu, uint16_t emulator_time) {
	apu->ch2.active = 1;
	apu->ch2.apu_div_start_time = apu->div_apu;
	apu->ch2.emulator_start_time = emulator_time;

	uint8_t nr21 = apu->bus->memory->NR21_LOCATION;
	uint8_t nr22 = apu->bus->memory->NR22_LOCATION;
	uint8_t nr23 = apu->bus->memory->NR23_LOCATION;
	uint8_t nr24 = apu->bus->memory->NR24_LOCATION;

	//If length timer expired, reset it
	if (apu->ch2.length_timer == 64) {
		//Initial length timer is bottom 5 bits of NR21
		apu->ch2.length_timer = nr21 & (0x1F);
	}

	//Bottom 3 bits of NR24 is upper 3 of period value
	//NR23 is bottom 8 bits of starting period
	apu->ch2.period_begin = ((nr24 & 0x7) << 8) | nr23;
	apu->ch2.period_div = apu->ch2.period_begin;

	//Envelope end tells how high the timer needs to get before adjusting volume
	//Timer starts at 0
	apu->ch2.env_overflow = nr22 & 0x7;
	apu->ch2.env_timer = 0;
	apu->ch1.env_direction = (nr22 >> 3) & 0x1; //Whether volume increases or decreases

	//Enable/Disable flags
	apu->ch2.length_enable = nr24 & (1 << 6) ? 1 : 0;
	apu->ch2.env_enable = apu->ch2.env_overflow ? 1 : 0;

	//Initial volume is top 4 bits of NR22
	apu->ch2.wave_volume = (nr22 & 0xF0) >> 4;
	apu->ch2.out = 0;
}

void activate_ch3(APU* apu, uint16_t emulator_time) {
	apu->ch3.active = 1;
	apu->ch3.current_sample = 1; //Index starts at 1
	apu->ch3.apu_div_start_time = apu->div_apu;
	apu->ch3.emulator_start_time = emulator_time;

	uint8_t nr30 = apu->bus->memory->NR30_LOCATION;
	uint8_t nr31 = apu->bus->memory->NR31_LOCATION;
	uint8_t nr32 = apu->bus->memory->NR32_LOCATION;
	uint8_t nr33 = apu->bus->memory->NR33_LOCATION;
	uint8_t nr34 = apu->bus->memory->NR34_LOCATION;

	//If length timer expired, reset it
	if (apu->ch3.length_timer >= 256) {
		//Initial length timer is NR31
		apu->ch3.length_timer = nr31;
	}

	//Bottom 3 bits of NR34 is upper 3 of period value
	//NR33 is bottom 8 bits of starting period
	apu->ch3.period_begin = ((nr34 & 0x7) << 8) | nr33;
	apu->ch3.period_div = apu->ch3.period_begin;

	//Enable/Disable flags
	apu->ch3.length_enable = nr34 & (1 << 6) ? 1 : 0;

	apu->ch3.out = 0;
}

void activate_ch4(APU* apu, uint16_t emulator_time) {
	apu->ch4.active = 1;
	apu->ch4.apu_div_start_time = apu->div_apu;
	apu->ch4.emulator_start_time = emulator_time;

	uint8_t nr41 = apu->bus->memory->NR41_LOCATION;
	uint8_t nr42 = apu->bus->memory->NR42_LOCATION;
	uint8_t nr43 = apu->bus->memory->NR43_LOCATION;
	uint8_t nr44 = apu->bus->memory->NR44_LOCATION;

	//If length timer expired, reset it
	if (apu->ch4.length_timer == 64) {
		//Initial length timer is bottom 5 bits of NR41
		apu->ch4.length_timer = nr41 & 0x1F;
	}

	//Envelope end tells how high the timer needs to get before adjusting volume
	//Timer starts at 0
	apu->ch4.env_overflow = nr42 & 0x7;
	apu->ch4.env_timer = 0;

	//LSFR determines volume that gets output. This is 0 when began
	//Sometimes this is 8 bit, sometimes 16 bit depending on the value of NR43
	apu->ch4.lfsr = 0;

	//Enable/Disable flags
	apu->ch4.length_enable = nr44 & (1 << 6) ? 1 : 0;
	apu->ch4.env_enable = apu->ch4.env_overflow ? 1 : 0;

	//Initial volume output
	apu->ch4.out = (nr42 >> 5) & 0x3;
}

//Deactivates channel
void deactivate_channel(APU* apu, ChannelNumber channel) {
	//Having this function is maybe less performance, but it makes the code a lot more readable,
	//and with such weird dependencies all over the place, I'll take that wherever I can get it.
	switch (channel) {
		case CHANNEL_1:
			apu->ch1.out = 0;
			apu->ch1.active = 0;
			break;

		case CHANNEL_2:
			apu->ch2.out = 0;
			apu->ch2.active = 0;
			break;

		case CHANNEL_3:
			apu->ch3.out = 0;
			apu->ch3.active = 0;
			break;

		case CHANNEL_4:
			apu->ch4.out = 0;
			apu->ch4.active = 0;

		//Exit for invalid channel
		default:
			return;
	}

	//Clear read-only bit in Audio Master Control (NR52)
	//Bit 0 is ch1, bit 1 is ch2, bit 2 is ch3, bit 3 is ch4
	apu->bus->memory->NR52_LOCATION &= ~(1 << channel);
}

void update_ch1(APU* apu, uint16_t emulator_time) {
	uint8_t apu_div_updated = apu->div_apu != apu->prev_div_apu; //Flag for whether or not APU DIV was updated

	//If these bits are clear, it turns the DAC off which turns the channel off immediately
	if (!(apu->bus->memory->NR12_LOCATION & (0xF8))) {
		deactivate_channel(apu, CHANNEL_1);
		return;
	}

	//If APU DIV was updated, also update the values tied to it
	if (apu_div_updated) {
		//Update timers tied to APU DIV
		uint8_t elapsed_apu_div_ticks = apu->div_apu - apu->ch1.apu_div_start_time; //Number of APU div ticks elapsed

		//Update length if timer is updated
		if (elapsed_apu_div_ticks % 2 == 0 && apu->ch1.length_enable) {
			++apu->ch1.length_timer; //Length timer increments every 2 APU DIV ticks

			//If length is expired, turn channel off
			//Channel 1 overflows at 64, and only gets reset when channel is retriggered
			if (apu->ch1.length_timer >= 64) {
				deactivate_channel(apu, CHANNEL_1);
				return;
			}
		}

		//Update frequency if sweep timer is updated
		if (elapsed_apu_div_ticks % 4 == 0 && apu->ch1.sweep_enable) {
			++apu->ch1.sweep_timer;

			//If timer passes overflow, update frequency
			if (apu->ch1.sweep_timer >= apu->ch1.sweep_overflow) {
				//If function returns 1, period overflowed and channel gets deactivated
				if (update_ch1_freq_sweep(apu)) {
					deactivate_channel(apu, CHANNEL_1);
					return;
				}
			}
		}

		//Update volume envelope
		if (elapsed_apu_div_ticks % 8 == 0 && apu->ch1.env_enable) {
			++apu->ch1.env_timer; //Envelope sweet increments every 8 APU DIV ticks

			//Update envelope if timer overflowed
			if (apu->ch1.env_timer >= apu->ch1.env_overflow)
				update_volume_envelope(apu, CHANNEL_1);
		}
	}

	//If period div was updated, check for overflow and take new sample if so
	//Period div increases every 4 dots
	uint16_t elapsed_emulator_time = emulator_time - apu->ch1.emulator_start_time;
	if (elapsed_emulator_time % 4 == 0) {
		++apu->ch1.period_div;

		//If div overflows, take new sample and reset it to values in NR13 and NR14
		//CH1 has 8 samples, so this will cycle it through 0 and 7
		if (apu->ch1.period_div >= 0x800) {
			apu->ch1.period_div = apu->ch1.period_begin;
			apu->ch1.current_sample = (apu->ch1.current_sample + 1) % apu->ch1.total_samples;
		}
	}

	//Returns either the wave high, or 0 depending on current step and duty cycle
	apu->ch1.out = apu->ch1.wave_volume * get_duty_cycle(apu, CHANNEL_1);
}

void update_ch2(APU* apu, uint16_t emulator_time) {
	uint8_t apu_div_updated = apu->div_apu != apu->prev_div_apu; //Flag for whether or not APU DIV was updated

	//If these bits are clear, it turns the DAC off which turns the channel off immediately
	if (!(apu->bus->memory->NR22_LOCATION & (0xF8))) {
		deactivate_channel(apu, CHANNEL_2);
		return;
	}

	//If APU DIV was updated, also update the values tied to it
	if (apu_div_updated) {
		//Update timers tied to APU DIV
		uint8_t elapsed_apu_div_ticks = apu->div_apu - apu->ch2.apu_div_start_time; //Number of APU div ticks elapsed

		//Update length if timer is updated
		if (elapsed_apu_div_ticks % 2 == 0 && apu->ch2.length_enable) {
			++apu->ch2.length_timer; //Length timer increments every 2 APU DIV ticks

			//If length is expired, turn channel off
			//Channel 2 overflows at 64, and only gets reset when channel is retriggered
			if (apu->ch2.length_timer >= 64) {
				deactivate_channel(apu, CHANNEL_2);
				return;
			}
		}

		//Update volume envelope
		if (elapsed_apu_div_ticks % 8 == 0 && apu->ch2.env_enable) {
			++apu->ch2.env_timer; //Envelope sweet increments every 8 APU DIV ticks

			//Update envelope if timer overflowed
			if (apu->ch2.env_timer >= apu->ch2.env_overflow)
				update_volume_envelope(apu, CHANNEL_2);
		}
	}

	//If period div was updated, check for overflow and take new sample if so
	//Period div increases every 4 dots
	uint16_t elapsed_emulator_time = emulator_time - apu->ch2.emulator_start_time;
	if (elapsed_emulator_time % 4 == 0) {
		++apu->ch2.period_div;

		//If div overflows, take new sample and reset it to values in NR23 and NR24
		//CH2 has 8 samples, so this will cycle it through 0 and 7
		if (apu->ch2.period_div >= 0x800) {
			apu->ch2.period_div = apu->ch2.period_begin;
			apu->ch2.current_sample = (apu->ch2.current_sample + 1) % apu->ch2.total_samples;
		}
	}

	//Returns either the wave high, or 0 depending on current step and duty cycle
	apu->ch2.out = apu->ch2.wave_volume * get_duty_cycle(apu, CHANNEL_2);
}

void update_ch3(APU* apu, uint16_t emulator_time) {
	uint8_t apu_div_updated = apu->div_apu != apu->prev_div_apu; //Flag for whether or not APU DIV was updated

	//If NR30 bit 7 is clear, it turns the DAC off which turns the channel off immediately
	if (!(apu->bus->memory->NR30_LOCATION >> 7)) {
		deactivate_channel(apu, CHANNEL_2);
		return;
	}

	//If DIV APU is updated, update the timers related to it
	if (apu_div_updated) {
		uint8_t elapsed_apu_div_ticks = apu->div_apu - apu->ch3.apu_div_start_time;

		//Update length timer
		if (elapsed_apu_div_ticks % 2 == 0 && apu->ch3.length_enable) {
			++apu->ch3.length_timer;

			//CH3 overflows at 256
			//If timer overflows, turn channel off
			if (apu->ch3.length_timer >= 256) {
				deactivate_channel(apu, CHANNEL_3);
				return;
			}
		}
	}

	//If period div is updated, check for overflow and take new sample
	uint16_t elapsed_emulator_time = emulator_time - apu->ch3.emulator_start_time;

	//Channel 3's period div updates every 2 dots instead of 4 for some god forsaken reason
	if (elapsed_emulator_time % 2 == 0) {
		++apu->ch3.period_div;

		//If DIV overflows, increase sample
		if (apu->ch3.period_div >= 0x800) {
			apu->ch3.period_div = apu->ch3.period_begin;
			apu->ch3.current_sample = (apu->ch3.current_sample + 1) % apu->ch3.total_samples;
		}
	}

	//Channel 3 reads from wave RAM instead of a duty cycle, so this will get the ouput volume
	//Index 0 and 1 read from 0xFF30, 2 and 3 from 0xFF31, and so on... 16 bytes total,
	//each "wave sample" is 1 nibble
	uint8_t wave_index = apu->ch3.current_sample / 2; //0 and 1 are 0, 2 and 3 are 1, etc...
	uint16_t wave_address_offset = 0x30 + wave_index;
	uint8_t out_byte = apu->bus->memory->io[wave_address_offset];
	uint8_t out = 0;

	//If the modulo is 0, read upper nibble, otherwise read lower...
	if (apu->ch3.current_sample % 2 == 0) {
		//Out is upper nibble
		out = (out_byte >> 4) & 0xF;
	}
	else {
		//Out is lower nibble
		out = out_byte & 0xF;
	}

	//FINALLY, Get output volume and adjust the out value based on that
	uint8_t out_vol = (apu->bus->memory->NR32_LOCATION >> 5) & 0x3;

	//If out_vol is 0, sound is muted, if its 1, its full volume, 2, 50%, 3, 25%
	if (out_vol = 0)
		out = 0;
	else
		out = out >> (out_vol - 1); //This is hardware accurate... if volume is 1, no shift. If its 2, shift once. If its 3, shift twice

	apu->ch3.out = out;
}

//Updates volume based on envelope timer
void update_volume_envelope(APU* apu, ChannelNumber channel) {
	//Value pointers
	uint8_t* env_timer = NULL;
	uint8_t* channel_volume = NULL;
	uint8_t* env_enable = NULL;
	
	//Static values
	uint8_t env_overflow = 0;
	uint8_t env_dir = 0;

	//Get channel values
	switch (channel) {
		case CHANNEL_1:
			env_timer = &apu->ch1.env_timer;
			channel_volume = &apu->ch1.wave_volume;
			env_enable = &apu->ch1.env_enable;

			env_overflow = apu->ch1.env_overflow;
			env_dir = apu->ch1.env_direction;
			break;

		case CHANNEL_2:
			env_timer = &apu->ch2.env_timer;
			channel_volume = &apu->ch2.wave_volume;
			env_enable = &apu->ch2.env_enable;

			env_overflow = apu->ch2.env_overflow;
			env_dir = apu->ch2.env_direction;
			break;

		case CHANNEL_4:
			env_timer = &apu->ch4.env_timer;
			channel_volume = &apu->ch4.wave_volume;
			env_enable = &apu->ch4.env_enable;

			env_overflow = apu->ch4.env_overflow;
			env_dir = apu->ch4.env_direction;
			break;

		//Exit if invalid channel
		default:
			return;
	}

	*env_timer -= env_overflow; //Resets timer
	if (env_dir != 0)
		*channel_volume = (*channel_volume + 1) % 16; //16 volume levels
	else
		--*channel_volume;

	//If envelope is 0, it is disabled
	if (*channel_volume == 0)
		*env_enable = 0;
}

//Updates channel 1's frequency sweep.
//It is the only channel that uses this
//Returns 1 if period overflows, meaning channel gets turned off, 0 otherwise
int update_ch1_freq_sweep(APU* apu) {
	uint16_t current_period = apu->ch1.period_begin; //'shadow' sweep value
	uint16_t period_diff = current_period >> apu->ch1.sweep_step; //Change in freq

	//Relevant memory locations
	uint8_t nr10 = apu->bus->memory->NR10_LOCATION;
	uint8_t nr14 = apu->bus->memory->NR14_LOCATION;

	//Add or subtract depending on if frequency increases or decreases
	if (apu->ch1.sweep_direction != 0) {
		//Avoid underflow
		if (current_period >= period_diff)
			current_period -= period_diff;
		else
			current_period = 0;
	}
	else
		current_period += period_diff;

	//Updates pace, which gets re-read after each sweep iteration
	apu->ch1.sweep_overflow = (nr10 >> 4) & 0x7;

	//If period overflows, turn channel off.
	if (current_period >= 0x800)
		return 1;

	//Otherwise, update frequency and register values to reflect
	apu->ch1.period_begin = current_period;
	apu->bus->memory->NR13_LOCATION = current_period & 0xFF; //Bottom 8 bits stored in NR13
	apu->bus->memory->NR14_LOCATION = (nr14 & 0xF8) | ((current_period >> 8) & 0x7); //Top 3 bits stored in bottom 3 of NR14

	return 0;
}

//Gets corresponding duty cycle for specified channel.
//This only applies to the pulse channels, which are channels 1 and 2
//This returns 1 for 'high' and 0 for 'low'. Default returns 1
uint8_t get_duty_cycle(APU* apu, ChannelNumber channel) {
	uint8_t step = 0;
	uint8_t duty_cycle_value = 0;

	switch (channel) {
		case CHANNEL_1:
			step = apu->ch1.current_sample;
			duty_cycle_value = (apu->bus->memory->NR11_LOCATION >> 6) & 0x3;
			break;
		
		case CHANNEL_2:
			step = apu->ch2.current_sample;
			duty_cycle_value = (apu->bus->memory->NR21_LOCATION >> 6) & 0x3;
			break;

		default:
			return 1;
	}

	//These are all next to each other in the array, so multiply 8 by the value and add the step its at
	return apu->duty_cycles[(8 * duty_cycle_value) + step];
}