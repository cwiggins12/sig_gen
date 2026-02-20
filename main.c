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

static int init_audio() {
    SDL_AudioSpec desired = {0};
    SDL_AudioSpec obtained = {0};

    desired.freq = 48000;
    desired.format = AUDIO_F32SYS;
    desired.channels = 1;
    desired.samples = 1024;
    desired.callback = audio_callback;

    if (SDL_OpenAudio(&desired, &obtained) < 0) {
        return 1;
    }

    signal_init(obtained.freq);
    signal_set_frequency(440.0f);
    signal_set_amplitude(0.5f);

    SDL_PauseAudio(0);
    return 0;
}

static int handle_keydown(SDL_Event* e) {
    switch (e->key.keysym.sym) {
        case SDLK_UP:
            signal_set_frequency(signal_get_frequency() + 10.0f);
            break;
        case SDLK_DOWN:
            signal_set_frequency(signal_get_frequency() - 10.0f);
            break;
        case SDLK_LEFT:
            signal_set_amplitude(signal_get_amplitude() - 0.05f);
            break;
        case SDLK_RIGHT:
            signal_set_amplitude(signal_get_amplitude() + 0.05f);
            break;
        case SDLK_0: signal_set_waveform(NONE); break;
        case SDLK_1: signal_set_waveform(WAVE_SINE); break;
        case SDLK_2: signal_set_waveform(WAVE_SQUARE); break;
        case SDLK_3: signal_set_waveform(WAVE_SAW); break;
        case SDLK_4: signal_set_waveform(WAVE_TRIANGLE); break;
        case SDLK_5: signal_set_waveform(WHITE_NOISE); break;
        case SDLK_6: signal_set_waveform(PINK_NOISE); break;
        case SDLK_q: return 0;
    }
    return 1;
}

static int handle_event_poll(struct Renderer* r, SDL_Event* e) {
    if (e->type == SDL_QUIT)
        return 0;

    /* Let renderer consume event first */
    if (renderer_handle_event(r, e))
        return 1;

    if (r->edit_mode != EDIT_NONE) 
        return 1;

    /* If not editing, allow global key controls */
    if (e->type == SDL_KEYDOWN)
        return handle_keydown(e);

    return 1;
}

static void shutdown(struct Renderer* r) {
    signal_shutdown();
    SDL_CloseAudio();
    renderer_shutdown(r);
    SDL_Quit();
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

    if (init_audio() != 0) {
        renderer_shutdown(&renderer);
        SDL_Quit();
        return 1;
    }

    int running = 1;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            running = handle_event_poll(&renderer, &event);
        }
        render_frame(&renderer);
    }

    shutdown(&renderer);
    return 0;
}

