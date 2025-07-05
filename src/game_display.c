#include "game_display.h"
#include "logging.h"

#define SCALE 4


//Setup SDL Window
DisplayData* sdl_init(int screen_width, int screen_height) {
	//Create display data pointer...
	DisplayData* dis = (DisplayData*)malloc(sizeof(DisplayData));

	if (!dis) {
		printError("Failed to initialize SDL");
		return NULL;
	}

	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		printError("Failed to initialize SDL");
		return NULL;
	}

	SDL_Window* window = SDL_CreateWindow("Testieee :3", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, screen_width * SCALE, screen_height * SCALE, SDL_WINDOW_SHOWN);

	if (!window) {
		printError("Error creating window");
		SDL_Quit();
		return NULL;
	}

	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_STREAMING, screen_width, screen_height);
	
	dis->window = window;
	dis->renderer = renderer;
	dis->texture = texture;
	dis->running = 1;
	dis->height = screen_height;
	dis->width = screen_width;

	return dis;
}

void sdl_destroy(DisplayData* display) {
	if (display == NULL)
		return;

	SDL_DestroyTexture(display->texture);
	SDL_DestroyRenderer(display->renderer);
	SDL_DestroyWindow(display->window);
	SDL_Quit();
}

void draw_buffer(DisplayData* display, uint32_t* framebuffer) {
	SDL_Event e;
	while (SDL_PollEvent(&e)) {
		if (e.type == SDL_QUIT)
			display->running = 0;
	}

	SDL_UpdateTexture(display->texture, NULL, framebuffer, display->width * sizeof(uint32_t));
	SDL_RenderClear(display->renderer);
	SDL_RenderCopy(display->renderer, display->texture, NULL, NULL);
	SDL_RenderPresent(display->renderer);
}
