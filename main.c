
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdint.h>

#include "renderer.h"
#include "signal.h"

#define WIDTH 800
#define HEIGHT 600

static void audio_callback(void *userdata, Uint8 *stream, int len)
{
    float *buffer = (float *)stream;
    int samples = len / sizeof(float);

    for (int i = 0; i < samples; ++i)
        buffer[i] = signal_next_sample() * 0.2f;
}

int main()
{
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
    float current_frequency = 440.0f;

    while (running)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
                running = 0;

            if (event.type == SDL_KEYDOWN)
            {
                switch (event.key.keysym.sym)
                {
                    case SDLK_UP:
                        current_frequency += 10.0f;
                        signal_set_frequency(current_frequency);
                        break;

                    case SDLK_DOWN:
                        current_frequency -= 10.0f;
                        if (current_frequency < 10.0f)
                            current_frequency = 10.0f;
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

        render_frame(
            renderer.pixel_buffer,
            renderer.width,
            renderer.height
        );

        SDL_UpdateTexture(
            renderer.texture,
            NULL,
            renderer.pixel_buffer,
            renderer.width * sizeof(uint32_t)
        );

        SDL_RenderClear(renderer.sdl_renderer);
        SDL_RenderCopy(renderer.sdl_renderer, renderer.texture, NULL, NULL);
        SDL_RenderPresent(renderer.sdl_renderer);
    }

    SDL_CloseAudio();
    renderer_shutdown(&renderer);
    SDL_Quit();

    return 0;
}
