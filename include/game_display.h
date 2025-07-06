#ifndef GAME_DISPLAY_H
#define GAME_DISPLAY_H

#include <stdint.h>
#include <stdio.h>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

//Placeholder for now?
typedef struct {
	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_Texture* texture;
	uint8_t running;
	int width;
	int height;

	float frame_rate;
	uint64_t time_counter; //Used for framerate stuff. Probably belongs elsewhere? Maybe not?
} SDL_Data;

SDL_Data* sdl_init(int, int);
void sdl_destroy(SDL_Data*);
void draw_buffer(SDL_Data*, uint32_t*);

#endif 