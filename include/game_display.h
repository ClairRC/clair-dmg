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
} DisplayData;

DisplayData* sdl_init(int, int);
void sdl_destroy(DisplayData*);
void draw_buffer(DisplayData*, uint32_t*);

#endif 