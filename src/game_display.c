#include "game_display.h"
#include "logging.h"

#include <SDL2/SDL.h>

#define SCREEN_WIDTH 160
#define SCREEN_HEIGHT 144
#define SCALE 4

//Setup SDL Window
void sdl_init() {
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		printf("Failed to initialize SDL: %s\n", SDL_GetError());
		return 1;
	}

	SDL_Window* window = SDL_CreateWindow("Testieee :3", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH * SCALE, SCREEN_HEIGHT * SCALE, SDL_WINDOW_SHOWN);

	if (!window) {
		printError("Error creating window");
		SDL_Quit();
		return 1;
	}

	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);


	uint32_t fb[SCREEN_WIDTH * SCREEN_HEIGHT] = { 0xFFFFFF };
	for (int y = 0; y < SCREEN_HEIGHT; ++y) {
		for (int x = 0; x < SCREEN_WIDTH; ++x) {
			int i = y * SCREEN_WIDTH + x;

			uint8_t color = (i / 50) % 4;
			uint8_t brightness = (255 - color * 85);
			fb[i] = (brightness << 16) | (brightness << 8) | brightness;
		}
	}

	SDL_Event e;
	int running = 1;
	while (1) {
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT)
				running = 0;
		}

		SDL_UpdateTexture(texture, NULL, fb, SCREEN_WIDTH * sizeof(uint32_t));
		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, texture, NULL, NULL);
		SDL_RenderPresent(renderer);
		SDL_Delay(16);
	}

	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}