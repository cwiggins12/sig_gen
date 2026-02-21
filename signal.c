#include "signal.h"
#include <time.h>

#define PI 3.14159265358979323846f

static int sample_rate = 48000;

static float target_frequency = 440.0f;
static float target_amplitude = 0.5f;

static float current_frequency = 440.0f;
static float current_amplitude = 0.5f;

static WaveType waveform = WAVE_SINE;
static int waveform_change = 0;
static WaveType next_waveform = NONE;

static float phase = 0.0f;

static float smoothing_coeff = 0.0f;

static SDL_AudioDeviceID device = 0;


static float pink_b0 = 0.0f;
static float pink_b1 = 0.0f;
static float pink_b2 = 0.0f;
static float pink_b3 = 0.0f;
static float pink_b4 = 0.0f;
static float pink_b5 = 0.0f;
static float pink_b6 = 0.0f;

static const char *waveform_names[] = {
    "NONE",
    "SINE",
    "SQUARE",
    "SAW",
    "TRIANGLE",
    "WHITE NOISE",
    "PINK NOISE"
};

static uint32_t rng_state = 0x12345678;

static uint32_t xorshift32() {
    uint32_t x = rng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    rng_state = x;
    return x;
}

static float rand_uniform() {
    return (xorshift32() >> 8) * (1.0f / 16777216.0f);
}

static float rand_gaussian(float stddev) {
    static int has_spare = 0;
    static float spare;

    if (has_spare) {
        has_spare = 0;
        return spare * stddev;
    }

    float u, v, s;

    do {
        u = rand_uniform() * 2.0f - 1.0f;
        v = rand_uniform() * 2.0f - 1.0f;
        s = u * u + v * v;
    } while (s >= 1.0f || s == 0.0f);

    s = sqrtf(-2.0f * logf(s) / s);

    spare = v * s;
    has_spare = 1;

    return stddev * (u * s);
}

static float generate_pink() {
    float white = rand_gaussian(0.25f);
    pink_b0 = 0.99886f * pink_b0 + white * 0.0555179f;
    pink_b1 = 0.99332f * pink_b1 + white * 0.0750759f;
    pink_b2 = 0.96900f * pink_b2 + white * 0.1538520f;
    pink_b3 = 0.86650f * pink_b3 + white * 0.3104856f;
    pink_b4 = 0.55000f * pink_b4 + white * 0.5329522f;
    pink_b5 = -0.7616f * pink_b5 - white * 0.0168980f;

    float pink = pink_b0 + pink_b1 + pink_b2 +
                 pink_b3 + pink_b4 + pink_b5 +
                 pink_b6 + white * 0.5362f;

    pink_b6 = white * 0.115926f;

    return pink * 0.11f;
}

void signal_set_frequency(float freq) {
    SDL_LockAudioDevice(device);
    if (freq < 20.0f)           target_frequency = 20.0f;
    else if (freq > 20000.0f)   target_frequency = 20000.0f;
    else                        target_frequency = freq;
    SDL_UnlockAudioDevice(device);
}

void signal_set_amplitude(float amp) {
    SDL_LockAudioDevice(device);
    if (amp < 0.0f)         target_amplitude = 0.0f;
    else if (amp > 1.0f)    target_amplitude = 1.0f;
    else                    target_amplitude = amp;
    SDL_UnlockAudioDevice(device);
}

void signal_set_waveform(WaveType type) {
    if (type == waveform) return;
    SDL_LockAudioDevice(device);
    next_waveform = type;
    waveform_change = 1;
    SDL_UnlockAudioDevice(device);
}

float signal_get_frequency() {
    return target_frequency;
}

float signal_get_amplitude() {
    return target_amplitude;
}

const char* signal_get_waveform() {
    if (waveform < 0 || waveform > 6) {
        return "UNKNOWN";
    }

    return waveform_names[waveform];
}

SDL_AudioDeviceID signal_get_device() {
    return device;
}

void signal_init(int sr, SDL_AudioDeviceID d) {
    sample_rate = sr;
    phase = 0.0f;
    device = d;

    current_frequency = target_frequency;
    current_amplitude = target_amplitude;

    smoothing_coeff = 1.0f - expf(-1.0f / (0.005f * sample_rate));

    rng_state = (uint32_t)time(NULL);

    pink_b0 = pink_b1 = pink_b2 = pink_b3 =
    pink_b4 = pink_b5 = pink_b6 = 0.0f;

    signal_set_frequency(440.0f);
    signal_set_amplitude(0.5f);
}

static float generate_wave_sample(float phase) {
    switch (waveform) {
        case NONE: return 0.0f;

        case WAVE_SINE:
            return sinf(phase);

        case WAVE_SQUARE:
            return (sinf(phase) >= 0.0f) ? 1.0f : -1.0f;

        case WAVE_SAW:
            return 2.0f * (phase / (2.0f * PI)) - 1.0f;

        case WAVE_TRIANGLE: {
            float normalized = phase / (2.0f * PI);
            return 2.0f * fabsf(2.0f * normalized - 1.0f) - 1.0f;
        }

        case WHITE_NOISE:
            return rand_gaussian(0.25f);

        case PINK_NOISE:
            return generate_pink();

        default:
            return 0.0f;
    }
}

float signal_next_sample(void) {
    float target_amp = (waveform_change) ? 0.0 : target_amplitude;
    current_frequency += (target_frequency - current_frequency) * smoothing_coeff;
    current_amplitude += (target_amp - current_amplitude) * smoothing_coeff;
    
    if (waveform_change && current_amplitude < 0.000001f) {
        waveform_change = 0;
        waveform = next_waveform;
    }

    float phase_increment = 2.0f * PI * current_frequency / (float)sample_rate;

    phase += phase_increment;

    if (phase >= 2.0f * PI) {
        phase -= 2.0f * PI;
    }

    float sample = generate_wave_sample(phase) * current_amplitude;

    if (fabsf(sample) < 1e-20f) {
        sample = 0.0f;
    }

    return sample;
}

