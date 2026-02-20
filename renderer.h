#ifndef RENDERER_H
#define RENDERER_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

struct Label {
    SDL_Texture *texture;
    int w;
    int h;
};

struct Renderer {
    int width;
    int height;

    SDL_Window *window;
    SDL_Renderer *sdl_renderer;
    TTF_Font *font;

    // Static help labels
    struct Label help_freq;
    struct Label help_amp;
    struct Label help_wave;
    struct Label help_quit;

    // Dynamic labels
    struct Label freq_label;
    struct Label amp_label;
    struct Label wave_label;

    // Cached values
    float last_freq;
    float last_amp;
    const char *last_wave;
};

int  renderer_init(struct Renderer *r, int width, int height);
void renderer_shutdown(struct Renderer *r);
void render_frame(struct Renderer *r);

#endif
