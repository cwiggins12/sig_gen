#include "renderer.h"
#include "signal.h"
#include <stdlib.h>
#include <stdint.h>

#define PI 3.14159265358979323846f

int renderer_init(struct Renderer *r, int width, int height)
{
    r->width = width;
    r->height = height;

    r->window = SDL_CreateWindow(
        "Signal Generator Scope",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width,
        height,
        0
    );

    if (!r->window) {
        printf("SDL_CreateWindow failed: %s\n", SDL_GetError());
        return -1;
    }

    r->sdl_renderer = SDL_CreateRenderer(
        r->window,
        -1,
        SDL_RENDERER_ACCELERATED
    );

    if (!r->sdl_renderer) {
        printf("SDL_CreateRenderer failed: %s\n", SDL_GetError());
        return -1;
    }

    r->texture = SDL_CreateTexture(
        r->sdl_renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        width,
        height
    );

    if (!r->texture) {
        printf("SDL_CreateTexture failed: %s\n", SDL_GetError());
        return -1;
    }

    r->pixel_buffer = malloc(width * height * sizeof(uint32_t));
    if (!r->pixel_buffer) {
        printf("Failed to allocate pixel buffer\n");
        return -1;
    }

    if (TTF_Init() != 0) {
        printf("TTF_Init failed: %s\n", TTF_GetError());
        return -1;
    }

    r->font = TTF_OpenFont(
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        18
    );

    if (!r->font) {
        printf("TTF_OpenFont failed: %s\n", TTF_GetError());
        return -1;
    }

    return 0;
}

void renderer_shutdown(struct Renderer *r)
{
    if (r->font) {
        TTF_CloseFont(r->font);
    }

    TTF_Quit();

    if (r->pixel_buffer) {
        free(r->pixel_buffer);
    }

    if (r->texture) {
        SDL_DestroyTexture(r->texture);
    }

    if (r->sdl_renderer) {
        SDL_DestroyRenderer(r->sdl_renderer);
    }

    if (r->window) {
        SDL_DestroyWindow(r->window);
    }
}

struct Surface {
    uint32_t *buffer;
    int width;
    int height;
};

static void clear_buffer(struct Surface *s, uint32_t color) {
    for (int i = 0; i < s->width * s->height; ++i) {
        s->buffer[i] = color;
    }
}

static void put_pixel(struct Surface *s, int x, int y, uint32_t color) {
    if (x < 0 || x >= s->width || y < 0 || y >= s->height) {
        return;
    }
    s->buffer[y * s->width + x] = color;
}

static void put_h_line(struct Surface *s, int x, int y, int length, uint32_t color) {
    for (int i = 0; i < length; ++i) {
        put_pixel(s, x + i, y, color);
    }
}

static void put_v_line(struct Surface *s, int x, int y, int length, uint32_t color) {
    for (int i = 0; i < length; ++i) {
        put_pixel(s, x, y + i, color);
    }
}

static void put_line(struct Surface *s, int x0, int y0, int x1, int y1, uint32_t color) {
    int dx = x1 - x0;
    int dy = y1 - y0;

    int steps = abs(dx) > abs(dy) ? abs(dx) : abs(dy);

    for (int i = 0; i < steps; ++i) {
        float t = (float)i / steps;
        int x = x0 + dx * t;
        int y = y0 + dy * t;
        put_pixel(s, x, y, color);
    }
}

static void put_rect(struct Surface *s, int x, int y, int w, int h, uint32_t color) {
    if (w <= 0 || h <= 0) {
        return;
    }

    put_h_line(s, x, y, w, color);
    put_h_line(s, x, y + h - 1, w, color);
    put_v_line(s, x, y, h, color);
    put_v_line(s, x + w - 1, y, h, color);
}

static void fill_rect(struct Surface *s, int x, int y, int w, int h, uint32_t color) {
    if (w <= 0 || h <= 0) {
        return;
    }

    for(int i = y; i < y + h; ++i) {
        put_h_line(s, x, i, w, color);
    }
}

static void draw_grid(struct Surface *s, int x_space, int y_space, uint32_t color) {
    for (int x = 0; x < s->width; x += x_space) {
        put_v_line(s, x, 0, s->height, color);
    }

    for (int y = 0; y < s->height; y += y_space) {
        put_h_line(s, 0, y, s->width, color);
    }
}

void render_frame(uint32_t *buffer, int width, int height) {
    struct Surface surface = {
        .buffer = buffer,
        .width = width,
        .height = height
    };

    clear_buffer(&surface, 0xFF000000);

    draw_grid(&surface, 100, 100, 0xFF303030);

    const float *sig_buffer = signal_get_buffer();
    int buffer_size = signal_get_buffer_size();

    float amplitude = height / 3.0f;
    float center = height / 2.0f;

    uint32_t wave_color = 0xFFFFFFFF;

    int write_index = signal_get_write_index();
    int start = write_index - width;
    if (start < 0) {
        start += buffer_size;
    }
    float sample = sig_buffer[start];

    int last_y = (int)(center - sample * amplitude);

    put_pixel(&surface, 0, last_y, wave_color);

    for (int x = 0; x < width; ++x) {
        int index = (start + x) % buffer_size;
        sample = sig_buffer[index];

        int y = (int)(center - sample * amplitude);
        
        if (x > 0) {
            put_line(&surface, x - 1, last_y, x, y, wave_color);
        }
        else {
            put_pixel(&surface, x, y, wave_color);
        }

        last_y = y;
    }
}

