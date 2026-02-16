#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include "renderer.h"
#include "signal.h"

#define WIDTH 800
#define HEIGHT 600

static void audio_callback(void *userdata, Uint8 *stream, int len) {
    float *buffer = (float *)stream;
    int samples = len / sizeof(float);

    for (int i = 0; i < samples; ++i)
    {
        buffer[i] = signal_next_sample() * 0.2f;
    }
}

int main() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        printf("SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow(
        "Signal Generator Scope",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WIDTH,
        HEIGHT,
        0
    );

    SDL_Renderer *renderer = SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_ACCELERATED
    );

    SDL_Texture *texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        WIDTH,
        HEIGHT
    );

    SDL_AudioSpec desired = {0};
    SDL_AudioSpec obtained = {0};

    desired.freq = 48000;
    desired.format = AUDIO_F32SYS;
    desired.channels = 1;
    desired.samples = 512;
    desired.callback = audio_callback;

    if (SDL_OpenAudio(&desired, &obtained) < 0) {
        printf("SDL_OpenAudio Error: %s\n", SDL_GetError());
        return 1;
    }

    signal_init(obtained.freq);
    signal_set_frequency(440.0f);

    SDL_PauseAudio(0);

    uint32_t *pixel_buffer = malloc(WIDTH * HEIGHT * sizeof(uint32_t));
    if (!pixel_buffer) {
        printf("Failed to allocate pixel buffer\n");
        return 1;
    }

    int running = 1;
    SDL_Event event;

    float current_frequency = 440.0f;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            }

            if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_UP:
                        current_frequency += 10.0f;
                        signal_set_frequency(current_frequency);
                        break;
                    
                    case SDLK_DOWN:
                        current_frequency -= 10.0f;
                        if (current_frequency < 10.0f) {
                            current_frequency = 10.0f;
                        }
                        signal_set_frequency(current_frequency);
                        break;

                    case SDLK_1:
                        signal_set_waveform(WAVE_SINE);
                        break;

                    case SDLK_2:
                        signal_set_waveform(WAVE_SQUARE);
                        break;

                    case SDLK_3:
                        signal_set_waveform(WAVE_SAW);
                        break;

                    case SDLK_4:
                        signal_set_waveform(WAVE_TRIANGLE);
                        break;
                }
            }
        }

        render_frame(pixel_buffer, WIDTH, HEIGHT);

        SDL_UpdateTexture(texture, NULL, pixel_buffer, WIDTH * sizeof(uint32_t));
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
    }

    SDL_CloseAudio();

    free(pixel_buffer);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

