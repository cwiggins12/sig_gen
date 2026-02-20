#include "renderer.h"
#include "signal.h"
#include <SDL2/SDL_render.h>
#include <stdio.h>

static int create_label(struct Renderer *r, struct Label *label, const char *text, SDL_Color color) {
    SDL_Surface *surface = TTF_RenderText_Blended(r->font, text, color);
    if (!surface) {
        printf("TTF_RenderText failed: %s\n", TTF_GetError());
        return -1;
    }

    label->texture = SDL_CreateTextureFromSurface(r->sdl_renderer, surface);
    if (!label->texture) {
        SDL_FreeSurface(surface);
        return -1;
    }

    label->w = surface->w;
    label->h = surface->h;

    SDL_FreeSurface(surface);
    return 0;
}

static void destroy_label(struct Label *l) {
    if (l->texture) {
        SDL_DestroyTexture(l->texture);
        l->texture = NULL;
    }
}

static void update_dynamic_labels(struct Renderer *r) {
    SDL_Color white = {255, 255, 255, 255};
    char text[64];

    float freq = signal_get_frequency();
    float amp = signal_get_amplitude();
    const char *wave = signal_get_waveform();

    if (freq != r->last_freq) {
        destroy_label(&r->freq_label);
        snprintf(text, sizeof(text), "Frequency: %.1f Hz", freq);
        create_label(r, &r->freq_label, text, white);
        r->last_freq = freq;
    }
    if (amp != r->last_amp) {
        destroy_label(&r->amp_label);
        snprintf(text, sizeof(text), "Amplitude: %.2f", amp);
        create_label(r, &r->amp_label, text, white);
        r->last_amp = amp;
    }
    if (wave != r->last_wave) {
        destroy_label(&r->wave_label);
        snprintf(text, sizeof(text), "Waveform: %s", wave);
        create_label(r, &r->wave_label, text, white);
        r->last_wave = wave;
    }
}

int renderer_init(struct Renderer *r, int width, int height) {
    r->width = width;
    r->height = height;

    r->window = SDL_CreateWindow("Signal Generator",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        width, height, 0);

    if (!r->window) return -1;

    r->sdl_renderer = SDL_CreateRenderer(r->window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (!r->sdl_renderer) return -1;

    if (TTF_Init() != 0) return -1;

    r->font =
        TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 18);

    if (!r->font) return -1;

    SDL_Color white = {255,255,255,255};

    create_label(r, &r->help_freq, "UP/DOWN: Change Frequency", white);
    create_label(r, &r->help_amp,  "LEFT/RIGHT: Change Amplitude", white);
    create_label(r, &r->help_wave, "0-6: Change Waveform", white);
    create_label(r, &r->help_quit, "Q: End Program", white);

    r->last_freq = -1.0f;
    r->last_amp  = -1.0f;
    r->last_wave = NULL;

    return 0;
}

void renderer_shutdown(struct Renderer *r) {
    destroy_label(&r->freq_label);
    destroy_label(&r->amp_label);
    destroy_label(&r->wave_label);

    destroy_label(&r->help_freq);
    destroy_label(&r->help_amp);
    destroy_label(&r->help_wave);
    destroy_label(&r->help_quit);

    if (r->font) {
        TTF_CloseFont(r->font);
    }
    TTF_Quit();

    if (r->sdl_renderer) {
        SDL_DestroyRenderer(r->sdl_renderer);
    }
    if (r->window) {
        SDL_DestroyWindow(r->window);
    }
}

void render_frame(struct Renderer *r) {
    SDL_SetRenderDrawColor(r->sdl_renderer, 0, 0, 0, 255);
    SDL_RenderClear(r->sdl_renderer);

    update_dynamic_labels(r);
    
    SDL_Rect dst;

    // Dynamic labels
    dst = (SDL_Rect){10, 10, r->freq_label.w, r->freq_label.h};
    SDL_RenderCopy(r->sdl_renderer, r->freq_label.texture, NULL, &dst);

    dst = (SDL_Rect){10, 40, r->amp_label.w, r->amp_label.h};
    SDL_RenderCopy(r->sdl_renderer, r->amp_label.texture, NULL, &dst);

    dst = (SDL_Rect){10, 70, r->wave_label.w, r->wave_label.h};
    SDL_RenderCopy(r->sdl_renderer, r->wave_label.texture, NULL, &dst);

    // Static help labels
    dst = (SDL_Rect){10, 130, r->help_freq.w, r->help_freq.h};
    SDL_RenderCopy(r->sdl_renderer, r->help_freq.texture, NULL, &dst);

    dst = (SDL_Rect){10, 160, r->help_amp.w, r->help_amp.h};
    SDL_RenderCopy(r->sdl_renderer, r->help_amp.texture, NULL, &dst);

    dst = (SDL_Rect){10, 190, r->help_wave.w, r->help_wave.h};
    SDL_RenderCopy(r->sdl_renderer, r->help_wave.texture, NULL, &dst);

    dst = (SDL_Rect){10, 220, r->help_quit.w, r->help_quit.h};
    SDL_RenderCopy(r->sdl_renderer, r->help_quit.texture, NULL, &dst);

    SDL_RenderPresent(r->sdl_renderer);
}

