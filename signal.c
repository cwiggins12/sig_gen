#include "signal.h"
#include <math.h>

#define PI 3.14159265358979323846f
#define BUFFER_SIZE 4096

static int g_sample_rate = 48000;
static float g_frequency = 440.0f;
static float g_amplitude = 0.5f;

static float g_phase = 0.0f;
static float g_phase_increment = 0.0f;

static float g_ring_buffer[BUFFER_SIZE];
static int g_write_index = 0;

static WaveType g_waveform = WAVE_SINE;

void signal_init(int sample_rate) {
    g_sample_rate = sample_rate;
    g_phase = 0.0f;
    g_write_index = 0;
    signal_set_frequency(g_frequency);
}

void signal_set_frequency(float freq) {
    g_frequency = freq;
    g_phase_increment = 2.0f * PI * g_frequency / (float)g_sample_rate;
}

void signal_set_amplitude(float amp) {
    g_amplitude = amp;
}

void signal_set_waveform(WaveType type) {
    g_waveform = type;
}

static float generate_wave_sample() {
    switch (g_waveform) {
        case WAVE_SINE:
            return sinf(g_phase);

        case WAVE_SQUARE:
            return (sinf(g_phase) >= 0.0f) ? 1.0f : -1.0f;

        case WAVE_SAW:
            return 2.0f * (g_phase / (2.0f * PI)) - 1.0f;

        case WAVE_TRIANGLE: {
            float normalized = g_phase / (2.0f * PI);
            return 2.0f * fabsf(2.0f * normalized - 1.0f) - 1.0f;
        }

        default:
            return 0.0f;
    }
}

float signal_next_sample(void) {
    float sample = generate_wave_sample();

    g_phase += g_phase_increment;
    if (g_phase >= 2.0f * PI) {
        g_phase -= 2.0f * PI;
    }

    g_ring_buffer[g_write_index] = sample;
    g_write_index = (g_write_index + 1) % BUFFER_SIZE;

    return sample;
}

int signal_get_write_index() {
    return g_write_index;
}

const float *signal_get_buffer(void) {
    return g_ring_buffer;
}

int signal_get_buffer_size(void) {
    return BUFFER_SIZE;
}

