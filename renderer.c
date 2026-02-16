#include "renderer.h"
#include "signal.h"
#include <math.h>
#include <stdint.h>

#define PI 3.14159265358979323846f

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
    int dx, dy, p, x, y;

    dx = x1 - x0;
    dy = y1 - y0;

    x = x0;
    y = y0;

    p = 2 * dy - dx;

    while (x < x1) {
        if (p >= 0) {
            put_pixel(s, x, y, color);
            y = y + 1;
            p = p + 2 * dy - 2 * dx;
        }
        else {
            put_pixel(s, x, y, color);
            p = p + 2 * dy;
        }
        x = x + 1;
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

    int index = (buffer_size - width + buffer_size) % buffer_size;
    float sample = sig_buffer[index];

    int last_y = (int)(center - sample * amplitude);

    put_pixel(&surface, 0, last_y, wave_color);

    for (int x = 0; x < width; ++x) {
        index = (buffer_size - width + x + buffer_size) % buffer_size;
        sample = sig_buffer[index];

        int y = (int)(center - sample * amplitude);

        put_line(&surface, x - 1, last_y, x, y, wave_color);

        last_y = y;
    }
}

