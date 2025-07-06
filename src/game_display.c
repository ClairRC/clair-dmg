#include "game_display.h"
#include "logging.h"


#define SCALE 4


//Setup SDL Window
SDL_Data* sdl_init(int screen_width, int screen_height) {
	//Create display data pointer...
	SDL_Data* dis = (SDL_Data*)malloc(sizeof(SDL_Data));

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
	dis->time_counter = 0; //Start this at 0...
	dis->frame_rate = 59.73;

	return dis;
}

void sdl_destroy(SDL_Data* display) {
	if (display == NULL)
		return;

	SDL_DestroyTexture(display->texture);
	SDL_DestroyRenderer(display->renderer);
	SDL_DestroyWindow(display->window);
	SDL_Quit();
}

void draw_buffer(SDL_Data* display, uint32_t* framebuffer) {
	SDL_UpdateTexture(display->texture, NULL, framebuffer, display->width * sizeof(uint32_t));
	SDL_RenderClear(display->renderer);
	SDL_RenderCopy(display->renderer, display->texture, NULL, NULL);
	SDL_RenderPresent(display->renderer);
	
	uint64_t start = display->time_counter;
	uint64_t end = SDL_GetPerformanceCounter();

	//If framerate is -1, that means unlimited fps
	//Otherwise, delay till the end of the frame in terms of real time
	if (display->frame_rate != -1) {
		double elapsed_ms = (end - start) * 1000.0 * (1.0 / SDL_GetPerformanceFrequency());
		double target_frame_time = (1000.0 / display->frame_rate);

		//printf("frame time: %.2f\n", elapsed_ms);

		if (elapsed_ms < target_frame_time) {
			SDL_Delay((Uint32)(target_frame_time - elapsed_ms));
		}
	}

	display->time_counter = SDL_GetPerformanceCounter();
}
