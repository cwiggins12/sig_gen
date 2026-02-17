#ifndef RENDERER_H
#define RENDERER_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdint.h>

struct Renderer {
    SDL_Window *window;
    SDL_Renderer *sdl_renderer;
    SDL_Texture *texture;
    TTF_Font *font;

    uint32_t *pixel_buffer;
    int width;
    int height;
};

int renderer_init(struct Renderer *r, int width, int height);
void renderer_shutdown(struct Renderer *r);
void render_frame(uint32_t *buffer, int width, int height);

#endif
