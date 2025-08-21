#include "sdl_data.h"
#include "logging.h"

#include <stdlib.h>

#define SCALE 4

//Setup SDL Window
SDL_Data* sdl_init(int screen_width, int screen_height) {
    //Create SDL stuff
    SDL_Data* data = (SDL_Data*)malloc(sizeof(SDL_Data));

    SDL_Display_Data* display_data = (SDL_Display_Data*)malloc(sizeof(SDL_Display_Data));
    SDL_Input_Data* input_data = (SDL_Input_Data*)calloc(1, sizeof(SDL_Input_Data)); //Input data struct
    SDL_Audio_Data* audio_data = (SDL_Audio_Data*)malloc(sizeof(SDL_Audio_Data));

    //Video
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printError("Failed to initialize SDL Video");
        sdl_destroy(data);
        return NULL;
    }

    SDL_Window* window = SDL_CreateWindow("ClairDMG", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, screen_width * SCALE, screen_height * SCALE, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_STREAMING, screen_width, screen_height);

    //Audio
    if (SDL_Init(SDL_INIT_AUDIO) != 0) {
        printError("Failed to initialize SDL Audio");
        sdl_destroy(data);
        return NULL;
    }

    SDL_AudioSpec want = (SDL_AudioSpec){
        .freq = 44100,
        .format = AUDIO_S16SYS,
        .silence = 0,
        .channels = 2,
        .samples = 2048,
        .callback = NULL
    };

    SDL_AudioDeviceID dev = SDL_OpenAudioDevice(NULL, 0, &want, NULL, 0);
    SDL_ClearQueuedAudio(dev);

	if (data == NULL || display_data == NULL || input_data == NULL || audio_data == NULL || !window || !renderer || !texture) {
		printError("Error creating window");
		sdl_destroy(data);
		return NULL;
	}

    data->display_data = display_data;
    data->input_data = input_data;
    data->audio_data = audio_data;

	data->display_data->window = window;
	data->display_data->renderer = renderer;
	data->display_data->texture = texture;
	data->display_data->height = screen_height;
	data->display_data->width = screen_width;
    data->display_data->time_counter = 0;

    //Default button values for unpressed
    data->input_data->button_state = 0x0F;
    data->input_data->dpad_state = 0x0F;

    data->audio_data->buffer = (int16_t*)calloc(4096, sizeof(int16_t));
    data->audio_data->buffer_index = 0;
    data->audio_data->dev = dev;

    SDL_PauseAudioDevice(data->audio_data->dev, 0);

	return data;
}

void sdl_destroy(SDL_Data* data) {
	if (data == NULL)
		return;

	if (data->display_data != NULL) {
		SDL_DestroyTexture(data->display_data->texture);
		SDL_DestroyRenderer(data->display_data->renderer);
		SDL_DestroyWindow(data->display_data->window);

		free(data->display_data);
	}

    if (data->input_data != NULL)
        free(data->input_data);

    if (data->audio_data != NULL) {
        free(data->audio_data->buffer);
        free(data->audio_data);
    }

	SDL_Quit();

	free(data);
}

void draw_buffer(SDL_Display_Data* data, uint32_t* framebuffer, uint16_t framerate) {
	SDL_UpdateTexture(data->texture, NULL, framebuffer, data->width * sizeof(uint32_t));
	SDL_RenderClear(data->renderer);
	SDL_RenderCopy(data->renderer, data->texture, NULL, NULL);
	SDL_RenderPresent(data->renderer);
	
    //Waits for framerate to catch up
    uint64_t start = data->time_counter;
    uint64_t end = SDL_GetPerformanceCounter();

    double elapsed_ms = (end - start) * 1000.0 * (1.0 / SDL_GetPerformanceFrequency());
    double target_frame_time = (1000.0 / framerate); //59.73fps is gameboy framerate


    if (elapsed_ms < target_frame_time) {
        SDL_Delay((Uint32)(target_frame_time - elapsed_ms));
    }

    data->time_counter = SDL_GetPerformanceCounter();
}

//Polls SDL events and updates input data
//Returns 1 if SDL is quit, 0 otherwise
uint8_t poll_events(SDL_Input_Data* input) {
    //Gets current button states
    uint8_t button_state = input->button_state;
    uint8_t dpad_state = input->dpad_state;

    SDL_Event e;

    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT)
            return 1;

        if (e.type == SDL_KEYDOWN) {
            //A button
            if (e.key.keysym.sym == SDLK_z)
                button_state &= ~(1 << 0);
            //B button
            else if (e.key.keysym.sym == SDLK_x)
                button_state &= ~(1 << 1);
            //Select
            else if (e.key.keysym.sym == SDLK_RSHIFT)
                button_state &= ~(1 << 2);
            //Start
            else if (e.key.keysym.sym == SDLK_RETURN)
                button_state &= ~(1 << 3);


            //Right
            else if (e.key.keysym.sym == SDLK_RIGHT)
                dpad_state &= ~(1 << 0);
            //Left
            else if (e.key.keysym.sym == SDLK_LEFT)
                dpad_state &= ~(1 << 1);
            //Up
            else if (e.key.keysym.sym == SDLK_UP)
                dpad_state &= ~(1 << 2);
            //Down
            else if (e.key.keysym.sym == SDLK_DOWN)
                dpad_state &= ~(1 << 3);


            //Fast forward hotkey
            else if (e.key.keysym.sym == SDLK_SPACE)
                input->fast_foward = 1;
        }

        if (e.type == SDL_KEYUP) {
            //A button
            if (e.key.keysym.sym == SDLK_z)
                button_state |= (1 << 0);
            //B button
            else if (e.key.keysym.sym == SDLK_x)
                button_state |= (1 << 1);
            //Select
            else if (e.key.keysym.sym == SDLK_RSHIFT)
                button_state |= (1 << 2);
            //Start
            else if (e.key.keysym.sym == SDLK_RETURN)
                button_state |= (1 << 3);


            //Right
            else if (e.key.keysym.sym == SDLK_RIGHT)
                dpad_state |= (1 << 0);
            //Left
            else if (e.key.keysym.sym == SDLK_LEFT)
                dpad_state |= (1 << 1);
            //Up
            else if (e.key.keysym.sym == SDLK_UP)
                dpad_state |= (1 << 2);
            //Down
            else if (e.key.keysym.sym == SDLK_DOWN)
                dpad_state |= (1 << 3);


            //Fast forward hotkey
            else if (e.key.keysym.sym == SDLK_SPACE)
                input->fast_foward = 0;
        }
    }

    input->button_state = button_state & 0x0F;
    input->dpad_state = dpad_state & 0x0F;

    return 0;
}

//Plays values in audio buffer
void play_audio_buffer(SDL_Audio_Data* data) {
    //Queue audio samples
    SDL_QueueAudio(data->dev, data->buffer, 4096 * sizeof(int16_t));
}

//Changes name of window
void change_window_name(SDL_Data* data, char* new_name) {
    SDL_SetWindowTitle(data->display_data->window, new_name);
}