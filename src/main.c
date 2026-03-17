#include "renderer.h"
#include "signal.h"

#define WIDTH 320
#define HEIGHT 300

static void audio_callback(void *userdata, Uint8 *stream, int len) {
    (void)userdata;
    float *buffer = (float *)stream;
    int channels = signal_get_channels();
    int frames = (len / sizeof(float)) / channels;

    for (int i = 0; i < frames; ++i) {
        float sample = signal_next_sample();
        for (int ch = 0; ch < channels; ++ch) {
            buffer[i * channels + ch] = sample;
        }
    }
}

static int init_audio() {
    SDL_AudioSpec desired = {0};
    SDL_AudioSpec obtained = {0};

    SDL_GetAudioDeviceSpec(0, 0, &desired);
    desired.format = AUDIO_F32SYS;
    desired.samples = 4096;
    desired.callback = audio_callback;

    SDL_AudioDeviceID audio_device = SDL_OpenAudioDevice(NULL, 0, &desired, &obtained, 0);
    if (audio_device == 0) {
        printf("Failed to open audio: %s \n", SDL_GetError());
        return 1;
    }

    signal_init(obtained.freq, obtained.channels, audio_device);
    SDL_PauseAudioDevice(audio_device, 0);
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
        case SDLK_ESCAPE: return 0;
    }
    return 1;
}

static int handle_event_poll(struct Renderer* r, SDL_Event* e) {
    if (e->type == SDL_QUIT) {
        signal_set_amplitude(0.0f);
        return 0;
    }

    if (renderer_handle_event(r, e)) {
        return 1;
    }

    if (r->edit_mode != EDIT_NONE) {
        return 1;
    }

    if (e->type == SDL_KEYDOWN) {
        int ret = handle_keydown(e);
        if (ret == 0) {
            signal_set_amplitude(0.0f);
        }
        return ret;
    }

    return 1;
}

static void shutdown(struct Renderer* r) {
    SDL_CloseAudioDevice(signal_get_device());
    renderer_shutdown(r);
    SDL_Quit();
}

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        return 1;
    }
    if (init_audio() != 0) {
        SDL_Quit();
        return 1;
    }
    struct Renderer renderer;
    if (renderer_init(&renderer, WIDTH, HEIGHT) != 0) {
        SDL_CloseAudioDevice(signal_get_device());
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
