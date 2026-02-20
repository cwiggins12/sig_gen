#include <SDL2/SDL.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>

#include "renderer.h"
#include "signal.h"

#define WIDTH 300
#define HEIGHT 260

static void audio_callback(void *userdata, Uint8 *stream, int len) {
    float *buffer = (float *)stream;
    int samples = len / sizeof(float);

    for (int i = 0; i < samples; ++i) {
        buffer[i] = signal_next_sample();
    }
}

int main() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        printf("SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    struct Renderer renderer;

    if (renderer_init(&renderer, WIDTH, HEIGHT) != 0) {
        SDL_Quit();
        return 1;
    }

    SDL_AudioSpec desired = {0};
    SDL_AudioSpec obtained = {0};

    desired.freq = 48000;
    desired.format = AUDIO_F32SYS;
    desired.channels = 1;
    desired.samples = 512;
    desired.callback = audio_callback;

    if (SDL_OpenAudio(&desired, &obtained) < 0) {
        renderer_shutdown(&renderer);
        SDL_Quit();
        return 1;
    }

    signal_init(obtained.freq);
    signal_set_frequency(440.0f);
    SDL_PauseAudio(0);

    int running = 1;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            }

            if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_UP: {
                        float f = signal_get_frequency();
                        signal_set_frequency(f + 10.0f);
                        break;
                    }
                    case SDLK_DOWN: {
                        float f = signal_get_frequency();
                        signal_set_frequency(f - 10.0f);
                        break;
                    }
                    case SDLK_LEFT: {
                        float a = signal_get_amplitude();
                        signal_set_amplitude(a - 0.05f);
                        break;
                    }
                    case SDLK_RIGHT: {
                        float a = signal_get_amplitude();
                        signal_set_amplitude(a + 0.05f);
                        break;
                    }
                    case SDLK_0: {
                        signal_set_waveform(NONE);
                        break;
                    }
                    case SDLK_1: {
                        signal_set_waveform(WAVE_SINE);
                        break;
                    }
                    case SDLK_2: {
                        signal_set_waveform(WAVE_SQUARE);
                        break;
                    }
                    case SDLK_3: {
                        signal_set_waveform(WAVE_SAW);
                        break;
                    }
                    case SDLK_4: {
                        signal_set_waveform(WAVE_TRIANGLE);
                        break;
                    }
                    case SDLK_5: {
                        signal_set_waveform(WHITE_NOISE);
                        break;
                    }
                    case SDLK_6: {
                        signal_set_waveform(PINK_NOISE);
                        break;
                    }
                    case SDLK_q: {
                        running = 0;
                        break;
                    }
                }
            }
        }

        render_frame(&renderer);
    }

    signal_shutdown();
    SDL_CloseAudio();
    renderer_shutdown(&renderer);
    SDL_Quit();

    return 0;
}

