#include "apu.h"
#include "logging.h"
#include <stdlib.h>

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

APU* apu_init(MemoryBus* bus, GlobalAPUState* global_state, SDL_Audio_Data* sdl_data) {
	APU* apu = (APU*)calloc(1, sizeof(APU));

	if (bus == NULL || global_state == NULL || apu == NULL) {
		printError("Error initializing APU");
		return NULL;
	}

	apu->bus = bus;
	apu->global_state = global_state;
	apu->sdl_data = sdl_data;

	//Local State values
	apu->local_state.ch1.length_timer_end = 64;
	apu->local_state.ch2.length_timer_end = 64;
	apu->local_state.ch3.length_timer_end = 256;
	apu->local_state.div_bit = 4; //Is 5 in double speed mode
	apu->local_state.prev_div_state = 0;
	apu->local_state.target_interval = 4194304.0 / 44100.0; //GB clock speed divided by sample rate gives number of cycles between samples
	apu->local_state.error_accumulator = 0.0;

	//Duty cycles
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

	//Copy duty cycle values over
	for (int i = 0; i < 32; ++i)
		apu->duty_cycles[i] = duty_cycles[i];

	return apu;
}

void apu_destroy(APU* apu) {
	if (apu != NULL)
		free(apu);
}

//Adds to SDL audio buffer
void fill_buffer(APU* apu) {
	APUSample sample = mix_dac_values(apu);
	
	apu->sdl_data->buffer[apu->sdl_data->buffer_index++] = sample.left; //Add sample to buffer
	apu->sdl_data->buffer[apu->sdl_data->buffer_index++] = sample.right;

	//If buffer is full, give it to SDL and reset the index
	if (apu->sdl_data->buffer_index >= 4096) {
		apu->sdl_data->buffer_index = 0;

		//Only queue audio if queue is running low-ish. This prevents sound being processed quicker than SDL can play it
		//This will reduce delay, but introduce some occasional popping
		//I chose an arbitrary number that seemed to be a good compromise between the two
		if (SDL_GetQueuedAudioSize(apu->sdl_data->dev) <= 26000) {
			play_audio_buffer(apu->sdl_data);
		}
	}
}

//Get DAC Values mixed together !
APUSample mix_dac_values(APU* apu) {
	APUSample sample = (APUSample){ .left = 0, .right = 0 };

	//If APU is off, output is 0 in both ears
	if (!apu->global_state->apu_enable)
		return sample;

	float channel_1_dac = 0;
	float channel_2_dac = 0;
	float channel_3_dac = 0;
	//float channel_4_dac = 0;

	//Max output volume is 15, so subtract 7.5 to have it centered at 0 before scaling
	if (apu->local_state.ch1.dac_enable && apu->local_state.ch1.out < 16)
		channel_1_dac = (float)apu->local_state.ch1.out - 7.5;
	if (apu->local_state.ch2.dac_enable && apu->local_state.ch2.out < 16)
		channel_2_dac = (float)apu->local_state.ch2.out - 7.5;
	if (apu->local_state.ch3.dac_enable && apu->local_state.ch3.out < 16)
		channel_3_dac = (float)apu->local_state.ch3.out - 7.5;
	//if (apu->local_state.ch4.dac_enable && apu->local_state.ch4.out < 16)
	//	channel_4_dac = (float)apu->local_state.ch4.out - 7.5;

	//Gets left and right outputs
	float left = 0;
	uint8_t left_vol = (apu->bus->memory->NR50_LOCATION >> 4) & 0x3; //NR50 has output volume for each
	float right = 0;
	uint8_t right_vol = apu->bus->memory->NR50_LOCATION & 0x3;
	uint8_t mix = apu->bus->memory->NR51_LOCATION; //This decides whether a sound gets played left, right, center, or not at all

	//Of each nibble, bit 0 determines where channel 1 is output, bit 1 is channel 2, bit 2 is channel 3, bit 3 is channel 4
	if (mix & 0x01) //Ch1 right
		right += channel_1_dac;
	if (mix & 0x10) //Ch1 left
		left += channel_1_dac;

	if (mix & 0x02) //Ch2 right
		right += channel_2_dac;
	if (mix & 0x20) //Ch2 left
		left += channel_2_dac;

	if (mix & 0x04) //Ch3 right
		right += channel_3_dac;
	if (mix & 0x40) //Ch3 left
		left += channel_3_dac;

	//if (mix & 0x08) //Ch4 right
	//	right += channel_4_dac;
	//if (mix & 0x80) //Ch4 left
	//	left += channel_4_dac;

	//Each channel can have an output of between -30 and 30 (4 * +/-7.5)
	//But the output is in 16-bit values, so scale each to 16 bit
	right /= 30; //Scale between -1 and 1
	left /= 30;

	right *= 32766; //Scale between -32766 and 32766
	left *= 32766;

	right *= (right_vol + 1) / 8.0; //Scale based on volume slider, which is a value from 0-8. This setting never mutes output
	left *= (left_vol + 1) / 8.0;

	sample.left = (int16_t)left;
	sample.right = (int16_t)right;

	return sample;
}

//Checks whether channels should be activated and updates DIV APU
void update_apu(APU* apu, uint64_t emulator_time) {
	//Every 95.2 t-cycles on average, fill audio buffer. This is approximately 44.1kHz
	//Accumulating error for each t-cycle will allow any extra cycles to be accounted for, so this should
	//average approximately 95.2 t-cycles per sample, which is approximately 44.1kHz with GB's clock speed
	apu->local_state.error_accumulator += 1.0; //Increment error accumulator
	if (apu->local_state.error_accumulator >= apu->local_state.target_interval) {
		apu->local_state.error_accumulator -= apu->local_state.target_interval;
		fill_buffer(apu);
	}

	//Update DIV APU for channel updates
	//This happens even when APU is off, as it is tied to DIV
	update_div_apu(apu);

	//Update DACs to see which channels should be updated
	update_dacs(apu);

	//Check if any channels have been triggered
	update_channel_active(apu, emulator_time);

	//If APU is to be turned off and current on, handle logic
	if (apu->global_state->turn_off_apu) {
		apu->global_state->turn_off_apu = 0; //Disable flag

		//If APU is not already off, turn it off
		if (apu->global_state->apu_enable == 0)
			turn_off_apu(apu);
	}

	// APU is off, no other updates are necessary
	if (!apu->global_state->apu_enable)
		return;

	//If channel DAC is active, update the channel's timers
	if (apu->local_state.ch1.dac_enable)
		update_ch1(apu, emulator_time);
	if (apu->local_state.ch2.dac_enable)
		update_ch2(apu, emulator_time);
	if (apu->local_state.ch3.dac_enable)
		update_ch3(apu, emulator_time);
	//if (apu->local_state.ch4.dac_enable && apu->local_state.ch4.enable)
	//	update_ch4(apu, emulator_time);
}

//Update Channel DACs
void update_dacs(APU* apu) {
	//Channel 1,2,4 are disabled if NRx2 & 0xF8 == 0
	//Channel 3 has its own flag
	//Turning off DAC turns off the channel, but not vice versa

	//Check channel 1
	if ((apu->bus->memory->NR12_LOCATION & 0xF8) == 0) {
		apu->local_state.ch1.dac_enable = 0;
		apu->local_state.ch1.enable = 0;
		apu->bus->memory->NR52_LOCATION &= ~(0x1); //Clear read only channel on bit
	}
	else
		apu->local_state.ch1.dac_enable = 1;

	//Check channel 2
	if ((apu->bus->memory->NR22_LOCATION & 0xF8) == 0) {
		apu->local_state.ch2.dac_enable = 0;
		apu->local_state.ch2.enable = 0;
		apu->bus->memory->NR52_LOCATION &= ~(0x2); //Clear read only channel on bit
	}
	else
		apu->local_state.ch2.dac_enable = 1;

	//Check channel 3
	if ((apu->bus->memory->NR30_LOCATION & 0x80) == 0) {
		apu->local_state.ch3.dac_enable = 0;
		apu->local_state.ch3.enable = 0;
		apu->bus->memory->NR52_LOCATION &= ~(0x4); //Clear read only channel on bit
	}
	else
		apu->local_state.ch3.dac_enable = 1;

	//Check channel 4
	if ((apu->bus->memory->NR42_LOCATION) == 0) {
		apu->local_state.ch4.dac_enable = 0;
		apu->local_state.ch4.enable = 0;
		apu->bus->memory->NR52_LOCATION &= ~(0x8); //Clear read only channel on bit
	}
	else
		apu->local_state.ch4.dac_enable = 1;
}

//Update DIV APU
void update_div_apu(APU* apu) {
	uint8_t current_div_bit = (apu->bus->memory->DIV_LOCATION >> apu->local_state.div_bit) & 0x1; //Bit 4/5 of DIV

	//If previos bit was 1 and new bit is 0, an update occurs, so increment APU DIV
	if (apu->local_state.prev_div_state == 1 && current_div_bit == 0) {
		++apu->local_state.apu_div;
		apu->local_state.apu_div_updated = 1;
	}
	else
		apu->local_state.apu_div_updated = 0;

	
	//Replace previous bit with new bit
	apu->local_state.prev_div_state = current_div_bit;
}

//Checks whether each channel has been triggered
void update_channel_active(APU* apu, uint64_t emulator_time) {
	//If channel was triggered, enable it if DAC is enabled
	if (apu->global_state->trigger_ch1) {
		apu->global_state->trigger_ch1 = 0; //Turn off trigger

		if (apu->local_state.ch1.dac_enable && apu->global_state->apu_enable == 1)
			enable_channel_1(apu, emulator_time);
	}

	if (apu->global_state->trigger_ch2) {
		apu->global_state->trigger_ch2 = 0; //Turn off trigger

		if (apu->local_state.ch2.dac_enable && apu->global_state->apu_enable == 1)
			enable_channel_2(apu, emulator_time);
	}

	if (apu->global_state->trigger_ch3) {
		apu->global_state->trigger_ch3 = 0; //Turn off trigger

		if (apu->local_state.ch3.dac_enable && apu->global_state->apu_enable == 1)
			enable_channel_3(apu, emulator_time);
	}

	if (apu->global_state->trigger_ch4 && apu->global_state->apu_enable == 1) {
		apu->global_state->trigger_ch4 = 0; //Turn off trigger

		if (apu->local_state.ch4.dac_enable)
			enable_channel_4(apu, emulator_time);
	}
}

//Turns off APU
void turn_off_apu(APU* apu) {
	//If APU is off, it resets all of the audio registers (except NR52) and makes them read-only
	apu->global_state->apu_enable = 0; //Disable APU

	//Clear audio registers
	apu->bus->memory->NR10_LOCATION = 0;
	apu->bus->memory->NR11_LOCATION = 0;
	apu->bus->memory->NR12_LOCATION = 0;
	apu->bus->memory->NR13_LOCATION = 0;
	apu->bus->memory->NR14_LOCATION = 0;
	apu->bus->memory->NR21_LOCATION = 0;
	apu->bus->memory->NR22_LOCATION = 0;
	apu->bus->memory->NR23_LOCATION = 0;
	apu->bus->memory->NR24_LOCATION = 0;
	apu->bus->memory->NR30_LOCATION = 0;
	apu->bus->memory->NR31_LOCATION = 0;
	apu->bus->memory->NR32_LOCATION = 0;
	apu->bus->memory->NR33_LOCATION = 0;
	apu->bus->memory->NR34_LOCATION = 0;
	apu->bus->memory->NR41_LOCATION = 0;
	apu->bus->memory->NR42_LOCATION = 0;
	apu->bus->memory->NR43_LOCATION = 0;
	apu->bus->memory->NR44_LOCATION = 0;
	apu->bus->memory->NR50_LOCATION = 0;
	apu->bus->memory->NR51_LOCATION = 0;
	apu->bus->memory->NR52_LOCATION &= 0x80; //Every bit except 7 gets cleared

	//Turn off DACs and channels
	apu->local_state.ch1.dac_enable = 0;
	apu->local_state.ch1.enable = 0;

	apu->local_state.ch2.dac_enable = 0;
	apu->local_state.ch2.enable = 0;
	
	apu->local_state.ch3.dac_enable = 0;
	apu->local_state.ch3.enable = 0;
	apu->local_state.ch3.last_sample = 0; //Last sample buffer gets reset for ch3

	apu->local_state.ch4.dac_enable = 0;
	apu->local_state.ch4.enable = 0;
}

//Enables audio channel 1 and sets initial values
void enable_channel_1(APU* apu, uint64_t emulator_time) {
	Ch1State* ch1 = &apu->local_state.ch1; //Chapter 1 state struct for more readability
	apu->bus->memory->NR52_LOCATION |= 0x1; //Set channel on bit

	//Sets start times
	ch1->emulator_time_start = emulator_time;
	ch1->sample_num = 0;

	//Enable channel 1
	ch1->enable = 1;

	//If length timer is expired, reset it
	if (ch1->length_timer >= ch1->length_timer_end)
		ch1->length_timer = apu->bus->memory->NR11_LOCATION & 0x3F; //Lower SIX!! bits of NR11 are initial timer value

	//Set period div and start time
	ch1->period_start = (uint16_t)(apu->bus->memory->NR14_LOCATION & 0x7) << 8; //Top 3 bits of period beginning value is bottom 3 of NR14
	ch1->period_start |= apu->bus->memory->NR13_LOCATION; //Bottom 8 bits of this is value of NR13
	ch1->period_div = ch1->period_start; //Period DIV starts at this

	//Set volume values
	ch1->high_vol = (apu->bus->memory->NR12_LOCATION >> 4) & 0xF; //Upper nibble of NR12 is initial value of "high" volume
	ch1->env_dir = (apu->bus->memory->NR12_LOCATION >> 3) & 0x1; //Bit 3 is direction of envelope. 0 for decrease, 1 for increase
	ch1->env_timer = 0; //This starts at 0
	ch1->env_end = apu->bus->memory->NR12_LOCATION & 0x7; //Bottom 3 bits of NR12 is how many times the timer has to tick before volume changes

	//Set frequency sweep values (ch1 specific)
	ch1->sweep_timer = 0;
	ch1->sweep_end = (apu->bus->memory->NR10_LOCATION >> 4) & 0x7; //Bits 4-6 are pace iterations
	ch1->sweep_dir = (apu->bus->memory->NR10_LOCATION >> 3) & 0x1; //Bit 3 is sweep direction
	ch1->sweep_step = apu->bus->memory->NR10_LOCATION & 0x7; //Bottom 3 bits are step value
	
	if (ch1->sweep_end != 0 || ch1->sweep_step != 0) //Sweep is enabled if sweep end or sweep step is non-zero
		ch1->sweep_enable = 1;
	else
		ch1->sweep_enable = 0;

	//If sweep is in addition mode, sweep step is non-zero, and WOULD overflow, the channel gets disabled instead for some reason
	if (ch1->sweep_step != 0 && ch1->sweep_dir == 0) {
		if (ch1->period_start + (ch1->period_start >> ch1->sweep_step) > 0x7FF) {
			ch1->enable = 0;
			apu->bus->memory->NR52_LOCATION &= ~(0x1); //Clear channel on bit
		}
	}
}

//Enables audio channel 2 and sets initial values
void enable_channel_2(APU* apu, uint64_t emulator_time) {
	Ch2State* ch2 = &apu->local_state.ch2; //Chapter 2 state struct for more readability
	apu->bus->memory->NR52_LOCATION |= 0x2; //Set channel on bit

	//Sets start times
	ch2->emulator_time_start = emulator_time;
	ch2->sample_num = 0;

	//Enable channel 2
	ch2->enable = 1;

	//If length timer is expired, reset it
	if (ch2->length_timer >= ch2->length_timer_end)
		ch2->length_timer = apu->bus->memory->NR21_LOCATION & 0x3F; //Lower 6 bits of NR21 are initial timer value

	//Set period div and start time
	ch2->period_start = (uint16_t)(apu->bus->memory->NR24_LOCATION & 0x7) << 8; //Top 3 bits of period beginning value is bottom 3 of NR24
	ch2->period_start |= apu->bus->memory->NR23_LOCATION; //Bottom 8 bits of this is value of NR23
	ch2->period_div = ch2->period_start; //Period DIV starts at this

	//Set volume values
	ch2->high_vol = (apu->bus->memory->NR22_LOCATION >> 4) & 0xF; //Upper nibble of NR22 is initial value of "high" volume
	ch2->env_dir = (apu->bus->memory->NR22_LOCATION >> 3) & 0x1; //Bit 3 is direction of envelope. 0 for decrease, 1 for increase
	ch2->env_timer = 0; //This starts at 0
	ch2->env_end = apu->bus->memory->NR22_LOCATION & 0x7; //Bottom 3 bits of NR22 is how many times the timer has to tick before volume changes
}

//Enables audio channel 2 and sets initial values
void enable_channel_3(APU* apu, uint64_t emulator_time) {
	Ch3State* ch3 = &apu->local_state.ch3;
	apu->bus->memory->NR52_LOCATION |= 0x4; //Set channel on bit

	//Enable channel 3
	ch3->emulator_time_start = emulator_time;
	ch3->enable = 1;

	//If length timer is expired, reset it
	if (ch3->length_timer >= ch3->length_timer_end)
		ch3->length_timer = apu->bus->memory->NR31_LOCATION; //NR31 has initial length timer

	//Set period start value
	ch3->period_start = (uint16_t)(apu->bus->memory->NR34_LOCATION & 0x7) << 8; //Top 3 bits of period beginning is bottom 3 of NR34
	ch3->period_start |= apu->bus->memory->NR33_LOCATION; //Bottom 8 bits of this is NR33
	ch3->period_div = ch3->period_start;

	//Reset wave RAM index. Starts at 1, as first sample with it on outputs the last sample read
	ch3->sample_num = 1;
}

//Enables channel 4 and sets initial values.
//TODO: Implement this
void enable_channel_4(APU* apu, uint64_t emulator_time) {}

//Updates channel 1 values based on timing
void update_ch1(APU* apu, uint64_t emulator_time) {
	//If channel is off, output is 0 (which doesn't necessarily mean muted!)
	if (apu->local_state.ch1.enable == 0) {
		apu->local_state.ch1.out = 0;
		return;
	}

	//This is kinda a long function, but the timing is so specific its hard to really decompose it more
	Ch1State* ch1 = &apu->local_state.ch1; //Ch1 struct for readability

	//If DIV-APU was updated, update relevant timers and states
	//DIV-APU determines envelope ticks, length timers, and CH1 freq sweep
	if (apu->local_state.apu_div_updated) {
		uint8_t div_apu = apu->local_state.apu_div; //How many elapsed APU

		//Every 2 ticks, length timer is updated if it is enabled, which is controlled by NR14 bit 6
		if (div_apu % 2 == 0 && apu->global_state->ch1_length_enable) {
			++ch1->length_timer;
			
			//If timer expires, turn channel off
			if (ch1->length_timer > ch1->length_timer_end) {
				ch1->enable = 0;
				apu->bus->memory->NR52_LOCATION &= ~(0x1); //Clears channel enable bit
			}
		}

		//Every 8 ticks, volume envelope is updated if it is enabled. It is enabled if "sweep pace" is 0
		if (div_apu % 8 == 0 && ch1->env_end != 0) {
			++ch1->env_timer;

			if (ch1->env_timer >= ch1->env_end) {
				if (ch1->env_dir == 0 && ch1->high_vol != 0)
					--ch1->high_vol; //Decrease high volume if it is not 0
				if (ch1->env_dir == 1 && ch1->high_vol != 15)
					++ch1->high_vol; //Increase high volume if it is not 15

				ch1->env_timer = 0; //Reset envelope timer
			}
		}

		//Every 4 ticks, freq sweep is updated
		//If pace is 0, then freq sweep is immediately disabled, so check that as well
		if (div_apu % 4 == 0 && (apu->bus->memory->NR10_LOCATION & 0x70)) {
			++ch1->sweep_timer;

			if (ch1->sweep_timer >= ch1->sweep_end) {
				uint16_t sweep_diff = ch1->period_start >> ch1->sweep_step; //Sweep difference

				//Depending on addition or subtraction mode, change period accordingly
				if (ch1->sweep_dir == 0) {
					//If period would overflow, then turn channel off, otherwise update period and write it back to registers
					if (ch1->period_start + sweep_diff > 0x7FF) {
						ch1->enable = 0;
						apu->bus->memory->NR52_LOCATION &= ~(0x1); //Clears channel enable bit
					}
					else {
						ch1->period_start += sweep_diff;
						apu->bus->memory->NR13_LOCATION = ch1->period_start & 0xFF; //Bottom 8 bits are in NR13
						apu->bus->memory->NR14_LOCATION &= ~(0x7); //Clear bottom 3 bits
						apu->bus->memory->NR14_LOCATION |= (ch1->period_start >> 8) & 0x7; //Store top 3 bits of period in bottom 3 of NR14
					}
				}

				else {
					//If period would underflow, set it to 0
					if (sweep_diff >= ch1->period_start)
						ch1->period_start = 0;
					else
						ch1->period_start -= sweep_diff;

					//Update period
					apu->bus->memory->NR13_LOCATION = ch1->period_start & 0xFF; //Bottom 8 bits are in NR13
					apu->bus->memory->NR14_LOCATION &= ~(0x7); //Clear bottom 3 bits
					apu->bus->memory->NR14_LOCATION |= (ch1->period_start >> 8) & 0x7; //Store top 3 bits of period in bottom 3 of NR14
				}
			}
		}
	}

	//If period div gets clocked, update relevant values
	//This happens every 4 dots
	if (((emulator_time - ch1->emulator_time_start) % 4) == 0) {
		++ch1->period_div; //Tick period div

		//If it overflows, reset it to period begin and change sample
		if (ch1->period_div > 0x7FF) {
			ch1->period_div = ch1->period_start;
			ch1->sample_num = (ch1->sample_num + 1) % 8; //Channel 1 has 8 samples, so this will loop
		}
	}

	//Finally, check duty cycle to see if output is high or low. This is bits 6 and 7 of NR11
	uint8_t wave_duty = (apu->bus->memory->NR11_LOCATION >> 6) & 0x3;

	//Set DAC output based on duty cycle
	if (apu->duty_cycles[(8 * wave_duty) + ch1->sample_num] == 1)
		ch1->out = ch1->high_vol;
	else
		ch1->out = 0;
}

//Updates channel 2 values based on timing
void update_ch2(APU* apu, uint64_t emulator_time) {
	//If channel is off, output is 0 (which doesn't necessarily mean muted!)
	if (apu->local_state.ch2.enable == 0) {
		apu->local_state.ch2.out = 0;
		return;
	}
	//This is kinda a long function, but the timing is so specific its hard to really decompose it more
	Ch2State* ch2 = &apu->local_state.ch2; //Ch2 struct for readability

	//If DIV-APU was updated, update relevant timers and states
	//DIV-APU determines envelope ticks, length timers, and CH1 freq sweep
	if (apu->local_state.apu_div_updated) {
		uint8_t div_apu = apu->local_state.apu_div; //How many elapsed APU ticks

		//Every 2 ticks, length timer is updated if it is enabled, which is controlled by NR24 bit 6
		if (div_apu % 2 == 0 && apu->global_state->ch2_length_enable) {
			++ch2->length_timer;

			//If timer expires, turn channel off
			if (ch2->length_timer > ch2->length_timer_end) {
				ch2->enable = 0;
				apu->bus->memory->NR52_LOCATION &= ~(0x2); //Clears channel enable bit
			}
		}

		//Every 8 ticks, volume envelope is updated if it is enabled. It is enabled if "sweep pace" is 0
		if (div_apu % 8 == 0 && ch2->env_end != 0) {
			++ch2->env_timer;

			if (ch2->env_timer >= ch2->env_end) {
				if (ch2->env_dir == 0 && ch2->high_vol != 0)
					--ch2->high_vol; //Decrease high volume if it is not 0
				if (ch2->env_dir == 1 && ch2->high_vol != 15)
					++ch2->high_vol; //Increase high volume if it is not 15

				ch2->env_timer = 0; //Reset envelope timer
			}
		}
	}

	//If period div gets clocked, update relevant values
	//This happens every 4 dots
	if (((emulator_time - ch2->emulator_time_start) % 4) == 0) {
		++ch2->period_div; //Tick period div

		//If it overflows, reset it to period begin and change sample
		if (ch2->period_div > 0x7FF) {
			ch2->period_div = ch2->period_start;
			ch2->sample_num = (ch2->sample_num + 1) % 8; //Channel 1 has 8 samples, so this will loop
		}
	}

	//Finally, check duty cycle to see if output is high or low. This is bits 6 and 7 of NR11
	uint8_t wave_duty = (apu->bus->memory->NR21_LOCATION >> 6) & 0x3;

	//Set DAC output based on duty cycle
	if (apu->duty_cycles[(8 * wave_duty) + ch2->sample_num] == 1)
		ch2->out = ch2->high_vol, 15;
	else
		ch2->out = 0;
}

//Updates channel 3 values based on timing
void update_ch3(APU* apu, uint64_t emulator_time) {
	//If channel is off, output is 0 (which doesn't necessarily mean muted!)
	if (apu->local_state.ch3.enable == 0) {
		apu->local_state.ch3.out = 0;
		return;
	}

	Ch3State* ch3 = &apu->local_state.ch3; //Ch3 struct for readability

	//If DIV-APU was updated, update relevant timers and states
	//DIV-APU determines envelope ticks, length timers, and CH1 freq sweep
	if (apu->local_state.apu_div_updated) {
		uint8_t div_apu = apu->local_state.apu_div; //How many elapsed APU ticks

		//Every 2 ticks, length timer is updated if it is enabled, which is controlled by NR34 bit 6
		if (div_apu % 2 == 0 && apu->global_state->ch3_length_enable) {
			++ch3->length_timer;

			//If timer expires, turn channel off
			if (ch3->length_timer > ch3->length_timer_end) {
				ch3->enable = 0;
				apu->bus->memory->NR52_LOCATION &= ~(0x4); //Clears channel enable bit
			}
		}
	}

	//If period div gets clocked, update relevant values
	//This happens every 2 dots for channel 3
	if (((emulator_time - ch3->emulator_time_start) % 2) == 0) {
		++ch3->period_div; //Tick period div

		//If it overflows, reset it to period begin and change sample
		if (ch3->period_div > 0x7FF) {
			ch3->period_div = ch3->period_start;
			ch3->sample_num = (ch3->sample_num + 1) % 32; //Channel 3 has 32 samples, so this will loop
		}
	}

	//Finally, read wave RAM for wave amplitude
	//Wave RAM is 16 registers. Sample 0 upper nibble, sample 1 is lower nibble, etc
	uint8_t wave_ram_offset = ch3->sample_num / 2;
	uint8_t wave_ram_value = apu->bus->memory->io[0x30 + wave_ram_offset]; //Gets wave ram byte
	uint8_t out_val = 0;

	if (ch3->sample_num % 2 == 0)
		out_val = (wave_ram_value >> 4) & 0xF; //If sample is even, read upper nibble
	else
		out_val = wave_ram_value & 0xF; //If sample is odd, read lower nibble

	//Bits 5 and 6 of NR32 have the output level
	uint8_t out_vol = (apu->bus->memory->NR32_LOCATION >> 5) & 0x3;

	//Depending on out volume, the wave amplitude gets affected
	if (out_vol == 0)
		out_val = 0; //Value of 0 mutes output
	if (out_vol == 2)
		out_val = out_val >> 1; //value of 2 gives 50% volume
	if (out_vol == 3)
		out_val = out_val >> 2; //Value of 3 gives 25% volume

	ch3->out = ch3->last_sample; //Output is whatever last sample was. This gets reset when APU is turned on
	ch3->last_sample = out_val; //Update sample
}

//Updates channel 4 values based on timing
//TODO: Implmenet this
void update_ch4(APU* apu, uint64_t emulator_time) {}