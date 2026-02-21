#ifndef RENDERER_H
#define RENDERER_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

struct Label {
    SDL_Texture *texture;
    int w;
    int h;
};

typedef enum {
    EDIT_NONE,
    EDIT_FREQ,
    EDIT_AMP
} EditMode;

struct Renderer {
    int width;
    int height;

    SDL_Window *window;
    SDL_Renderer *sdl_renderer;
    TTF_Font *font;

    struct Label help_freq;
    struct Label help_amp;
    struct Label help_type;
    struct Label help_wave;
    struct Label help_quit;

    struct Label freq_label;
    struct Label amp_label;
    struct Label wave_label;

    float last_freq;
    float last_amp;
    const char *last_wave;

    SDL_Rect freq_rect;
    SDL_Rect amp_rect;

    EditMode edit_mode;
    char edit_buffer[32];
};

int renderer_init(struct Renderer *r, int width, int height);
void renderer_shutdown(struct Renderer *r);
void render_frame(struct Renderer *r);

/* Returns 1 if event was consumed */
int renderer_handle_event(struct Renderer *r, SDL_Event *e);

#endif
