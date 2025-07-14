#ifndef SDL_DATA_H
#define SDL_DATA_H

#include <stdint.h>
#include <stdio.h>

#define SDL_MAIN_HANDLED
#include "SDL.h"

//SDL Visual Data
typedef struct {
	//Rendering
	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_Texture* texture;
	int width;
	int height;
} SDL_Display_Data;

//SDL Audio Data
typedef struct {
	//Audio
	SDL_AudioStream* stream;
	SDL_AudioSpec spec;
	SDL_AudioDeviceID dev;
} SDL_Audio_Data;

//SDL Input data
typedef struct {
	//Button and D-pad state are separate for hardware accuracy
	uint8_t button_state;
	uint8_t dpad_state;
	uint8_t fast_foward;
} SDL_Input_Data;

//Struct for all SDL data
typedef struct {
	SDL_Display_Data* display_data;
	SDL_Audio_Data* audio_data;
	SDL_Input_Data* input_data;
} SDL_Data;

SDL_Data* sdl_init(int screen_width, int screen_height);
void sdl_destroy(SDL_Data* data);
void draw_buffer(SDL_Display_Data* data, uint32_t* framebuffer);
uint8_t poll_events(SDL_Input_Data* input);

#endif 