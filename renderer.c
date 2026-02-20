#include "renderer.h"
#include "signal.h"
#include <SDL2/SDL_render.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

extern const unsigned char embedded_font[];
extern const unsigned int embedded_font_len;

static int create_label(struct Renderer *r, struct Label *label,
                        const char *text, SDL_Color color) {
    SDL_Surface *surface = TTF_RenderText_Blended(r->font, text, color);
    if (!surface) return -1;

    label->texture = SDL_CreateTextureFromSurface(r->sdl_renderer, surface);

    label->w = surface->w;
    label->h = surface->h;

    SDL_FreeSurface(surface);
    return label->texture ? 0 : -1;
}

static void destroy_label(struct Label *l) {
    if (l->texture) {
        SDL_DestroyTexture(l->texture);
        l->texture = NULL;
    }
}

static void update_dynamic_labels(struct Renderer *r) {
    SDL_Color white  = {255,255,255,255};
    SDL_Color yellow = {255,255,0,255};
    char text[64];

    const char *wave = signal_get_waveform();

    destroy_label(&r->freq_label);
    destroy_label(&r->amp_label);

    if (r->edit_mode == EDIT_FREQ) {
        if (strlen(r->edit_buffer) == 0)
            create_label(r, &r->freq_label, "Frequency: ", yellow);
        else {
            char text[64];
            snprintf(text, sizeof(text), "Frequency: %s", r->edit_buffer);
            create_label(r, &r->freq_label, text, yellow);
        }
    } else {
        char text[64];
        snprintf(text, sizeof(text), "Frequency: %.1f Hz", signal_get_frequency());
        create_label(r, &r->freq_label, text, white);
    }
    
    if (r->edit_mode == EDIT_AMP) {
        if (strlen(r->edit_buffer) == 0)
            create_label(r, &r->amp_label, "Amplitude: ", yellow);
        else {
            char text[64];
            snprintf(text, sizeof(text), "Amplitude: %s", r->edit_buffer);
            create_label(r, &r->amp_label, text, yellow);
        }
    } else {
        char text[64];
        snprintf(text, sizeof(text), "Amplitude: %.2f", signal_get_amplitude());
        create_label(r, &r->amp_label, text, white);
    }

    if (wave != r->last_wave) {
        destroy_label(&r->wave_label);
        snprintf(text, sizeof(text), "Waveform: %s", wave);
        create_label(r, &r->wave_label, text, white);
        r->last_wave = wave;
    }
}

int renderer_handle_event(struct Renderer *r, SDL_Event *e) {
    /* ---------------- Mouse Click ---------------- */
    if (e->type == SDL_MOUSEBUTTONDOWN) {
        SDL_Point p = { e->button.x, e->button.y };

        if (SDL_PointInRect(&p, &r->freq_rect)) {
            r->edit_mode = EDIT_FREQ;
            r->edit_buffer[0] = '\0';
            SDL_StartTextInput();
            return 1;
        }

        if (SDL_PointInRect(&p, &r->amp_rect)) {
            r->edit_mode = EDIT_AMP;
            r->edit_buffer[0] = '\0';
            SDL_StartTextInput();
            return 1;
        }

        /* Clicked elsewhere → cancel edit */
        r->edit_mode = EDIT_NONE;
        SDL_StopTextInput();
        return 0;
    }

    /* ---------------- Editing Mode ---------------- */
    if (r->edit_mode != EDIT_NONE) {

        /* Text input */
        if (e->type == SDL_TEXTINPUT) {
            strncat(r->edit_buffer,
                    e->text.text,
                    sizeof(r->edit_buffer) - strlen(r->edit_buffer) - 1);
            return 1;
        }

        if (e->type == SDL_KEYDOWN) {

            /* Backspace support */
            if (e->key.keysym.sym == SDLK_BACKSPACE) {
                size_t len = strlen(r->edit_buffer);
                if (len > 0) {
                    r->edit_buffer[len - 1] = '\0';
                }
                return 1;
            }

            /* Commit */
            if (e->key.keysym.sym == SDLK_RETURN) {

                char *endptr;
                float value = strtof(r->edit_buffer, &endptr);

                int valid_number =
                    (r->edit_buffer[0] != '\0') &&
                    (*endptr == '\0');

                if (valid_number) {

                    if (r->edit_mode == EDIT_FREQ) {
                        if (value >= 0.0f && value <= 20000.0f) {
                            signal_set_frequency(value);
                        }
                    }
                    else if (r->edit_mode == EDIT_AMP) {
                        if (value >= 0.0f && value <= 1.0f) {
                            signal_set_amplitude(value);
                        }
                    }
                }

                r->edit_mode = EDIT_NONE;
                SDL_StopTextInput();
                return 1;
            }

            /* Cancel */
            if (e->key.keysym.sym == SDLK_ESCAPE) {
                r->edit_mode = EDIT_NONE;
                SDL_StopTextInput();
                return 1;
            }
        }

        return 1; /* consume all events while editing */
    }

    return 0;
}

int renderer_init(struct Renderer *r, int width, int height) {
    r->width = width;
    r->height = height;
    r->edit_mode = EDIT_NONE;

    r->window = SDL_CreateWindow("Signal Generator",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        width, height, 0);

    if (!r->window) return -1;

    r->sdl_renderer = SDL_CreateRenderer(r->window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (!r->sdl_renderer) return -1;

    if (TTF_Init() != 0) return -1;

    //r->font = TTF_OpenFont("/home/cody/Projects/signal_generator/DS-DIGIB.TTF", 18);
    SDL_RWops *rw = SDL_RWFromConstMem(embedded_font, embedded_font_len);

    if (!rw) {
        printf("RWFromConstMem failed: %s\n", SDL_GetError());
        return -1;
    }

    r->font = TTF_OpenFontRW(rw, 1, 18);

    if (!r->font) {
        printf("TTF_OpenFontRW failed: %s\n", TTF_GetError());
        return -1;
    }

    SDL_Color white = {255,255,255,255};

    create_label(r, &r->help_freq, "UP/DOWN: Change Frequency", white);
    create_label(r, &r->help_amp,  "LEFT/RIGHT: Change Amplitude", white);
    create_label(r, &r->help_wave, "0-6: Change Waveform", white);
    create_label(r, &r->help_quit, "Q: End Program", white);

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

    if (r->font) TTF_CloseFont(r->font);
    TTF_Quit();

    if (r->sdl_renderer) SDL_DestroyRenderer(r->sdl_renderer);
    if (r->window) SDL_DestroyWindow(r->window);
}

void render_frame(struct Renderer *r) {

    SDL_SetRenderDrawColor(r->sdl_renderer, 0, 0, 0, 255);
    SDL_RenderClear(r->sdl_renderer);

    update_dynamic_labels(r);

    r->freq_rect = (SDL_Rect){10,10,r->freq_label.w,r->freq_label.h};
    SDL_RenderCopy(r->sdl_renderer, r->freq_label.texture, NULL, &r->freq_rect);

    r->amp_rect = (SDL_Rect){10,40,r->amp_label.w,r->amp_label.h};
    SDL_RenderCopy(r->sdl_renderer, r->amp_label.texture, NULL, &r->amp_rect);

    SDL_Rect dst;

    dst = (SDL_Rect){10,70,r->wave_label.w,r->wave_label.h};
    SDL_RenderCopy(r->sdl_renderer, r->wave_label.texture,NULL,&dst);

    dst = (SDL_Rect){10,130,r->help_freq.w,r->help_freq.h};
    SDL_RenderCopy(r->sdl_renderer, r->help_freq.texture,NULL,&dst);

    dst = (SDL_Rect){10,160,r->help_amp.w,r->help_amp.h};
    SDL_RenderCopy(r->sdl_renderer, r->help_amp.texture,NULL,&dst);

    dst = (SDL_Rect){10,190,r->help_wave.w,r->help_wave.h};
    SDL_RenderCopy(r->sdl_renderer, r->help_wave.texture,NULL,&dst);

    dst = (SDL_Rect){10,220,r->help_quit.w,r->help_quit.h};
    SDL_RenderCopy(r->sdl_renderer, r->help_quit.texture,NULL,&dst);

    SDL_RenderPresent(r->sdl_renderer);
}
