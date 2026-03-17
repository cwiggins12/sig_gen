#include "signal.h"
#include "fir_coeffs.h"
#include <time.h>
#include <math.h>

#define TWO_PI 6.28318530717958647692
#define RAMP_TIME_S 0.01f
#define OVERSAMPLE_FACTOR 4

static int output_sample_rate   = 48000;
static int internal_sample_rate = 48000 * OVERSAMPLE_FACTOR;
static int channels = 2;

static float target_frequency = 440.0f;
static float current_frequency = 440.0f;
static float freq_ratio = 1.0f;
static int freq_steps_remaining = 0;

static float target_amplitude = 0.5f;
static float current_amplitude = 0.5f;
static float amp_increment = 0.0f;
static int amp_steps_remaining = 0;

static WaveType waveform = WAVE_SINE;
static int waveform_pending = 0;
static WaveType next_waveform = NONE;

//counter-based phase: phase is computed as (sample_counter * current_frequency / internal_sample_rate).
//phase_offset is used to ensure phase continuity when frequency changes mid-stream.
//both operate at the internal (oversampled) rate.
static uint64_t sample_counter = 0;
static double phase_offset = 0.0;

static SDL_AudioDeviceID device = 0;

static float pink_b0 = 0.0f;
static float pink_b1 = 0.0f;
static float pink_b2 = 0.0f;
static float pink_b3 = 0.0f;
static float pink_b4 = 0.0f;
static float pink_b5 = 0.0f;
static float pink_b6 = 0.0f;

//circular buffer for FIR filter delay line.
static float fir_buf[FIR_TAPS];
static int   fir_buf_pos = 0;

static float fir_process(float x) {
    fir_buf[fir_buf_pos] = x;
    float acc = 0.0f;
    int pos = fir_buf_pos;
    for (int i = 0; i < FIR_TAPS; i++) {
        acc += fir_coeffs[i] * fir_buf[pos];
        if (--pos < 0) pos = FIR_TAPS - 1;
    }
    if (++fir_buf_pos >= FIR_TAPS) fir_buf_pos = 0;
    return acc;
}

static const char *waveform_names[] = {
    "NONE", "SINE", "SQUARE", "SAW", "TRIANGLE", "WHITE NOISE", "PINK NOISE"
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
    if (has_spare) { has_spare = 0; return spare * stddev; }
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
    float white = rand_gaussian(0.2f);
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
    //scalar found through testing to roughly cover some of the rms difference without hitting the clamp too hard
    //the commented out test can give a better scalar to better match the rms of the other signals if wanted
    return pink * 0.325f;
}

//corrects a unit-step discontinuity (used for saw and square)
static float polyblep(float phase, float phase_increment) {
    if (phase < phase_increment) {
        float t = phase / phase_increment;
        return t + t - t * t - 1.0f;
    } else if (phase > 1.0f - phase_increment) {
        float t = (phase - 1.0f) / phase_increment;
        return t * t + t + t + 1.0f;
    }
    return 0.0f;
}

//corrects a slope discontinuity (used for triangle)
static float polyblamp(float phase, float phase_increment) {
    if (phase < phase_increment) {
        float t = phase / phase_increment - 1.0f;
        return -1.0f / 3.0f * t * t * t;
    } else if (phase > 1.0f - phase_increment) {
        float t = (phase - 1.0f) / phase_increment + 1.0f;
        return 1.0f / 3.0f * t * t * t;
    }
    return 0.0f;
}

//returns the current phase in [0, 1) without accumulation error
static double get_phase() {
    double phase = phase_offset + (double)current_frequency * (double)sample_counter / (double)internal_sample_rate;
    phase -= floor(phase);
    return phase;
}

static void start_freq_ramp(float from, float to) {
    if (from == to) {
        current_frequency = to;
        freq_steps_remaining = 0;
        return;
    }
    double current_phase = get_phase();
    freq_steps_remaining = (int)(RAMP_TIME_S * internal_sample_rate);
    freq_ratio = powf(to / from, 1.0f / (float)freq_steps_remaining);
    phase_offset = current_phase;
    sample_counter = 0;
}

static void start_amp_ramp(float from, float to) {
    if (from == to) {
        current_amplitude = to;
        amp_steps_remaining = 0;
        return;
    }
    amp_steps_remaining = (int)(RAMP_TIME_S * internal_sample_rate);
    amp_increment = (to - from) / (float)amp_steps_remaining;
}

void signal_set_frequency(float freq) {
    if (freq < 20.0f)           target_frequency = 20.0f;
    else if (freq > 20000.0f)   target_frequency = 20000.0f;
    else                        target_frequency = freq;
    start_freq_ramp(current_frequency, target_frequency);
}

void signal_set_amplitude(float amp) {
    if (amp < 0.0f)         target_amplitude = 0.0f;
    else if (amp > 1.0f)    target_amplitude = 1.0f;
    else                    target_amplitude = amp;
    if (!waveform_pending) {
        start_amp_ramp(current_amplitude, target_amplitude);
    }
}

void signal_set_waveform(WaveType type) {
    if (type == waveform) return;
    next_waveform = type;
    waveform_pending = 1;
    start_amp_ramp(current_amplitude, 0.0f);
}

float signal_get_frequency()          { return target_frequency; }
float signal_get_amplitude()          { return target_amplitude; }
int   signal_get_channels()           { return channels; }
SDL_AudioDeviceID signal_get_device() { return device; }

const char* signal_get_waveform() {
    if (waveform < 0 || waveform > 6) return "UNKNOWN";
    return waveform_names[waveform];
}

void signal_init(int sr, int ch, SDL_AudioDeviceID d) {
    output_sample_rate   = sr;
    internal_sample_rate = sr * OVERSAMPLE_FACTOR;
    channels = ch;
    device = d;

    sample_counter = 0;
    phase_offset = 0.0;

    current_frequency = target_frequency;
    freq_steps_remaining = 0;
    freq_ratio = 1.0f;

    current_amplitude = target_amplitude;
    amp_steps_remaining = 0;
    amp_increment = 0.0f;

    waveform_pending = 0;

    rng_state = (uint32_t)time(NULL);

    pink_b0 = pink_b1 = pink_b2 = pink_b3 =
    pink_b4 = pink_b5 = pink_b6 = 0.0f;

    for (int i = 0; i < FIR_TAPS; i++) fir_buf[i] = 0.0f;
    fir_buf_pos = 0;

    // // Noise RMS measurement
    // FILE *rms_log = fopen("rms.txt", "w");
    // float sum_sq_white = 0.0f;
    // float sum_sq_pink = 0.0f;
    // int n = sr; // 1 second
    // for (int i = 0; i < n; i++) {
    //     float w = rand_gaussian(0.25f);
    //     float p = generate_pink();
    //     sum_sq_white += w * w;
    //     sum_sq_pink  += p * p;
    // }
    // float rms_white = sqrtf(sum_sq_white / n);
    // float rms_pink  = sqrtf(sum_sq_pink / n);
    // fprintf(rms_log, "white RMS: %f\n", rms_white);
    // fprintf(rms_log, "pink RMS: %f\n", rms_pink);
    // fprintf(rms_log, "scalar: %f\n", rms_white / rms_pink);
    // fclose(rms_log);

    // // reset pink
    // pink_b0 = pink_b1 = pink_b2 = pink_b3 =
    // pink_b4 = pink_b5 = pink_b6 = 0.0f;
}

static float generate_wave_sample(float phase, float phase_increment) {
    switch (waveform) {
        case NONE:      return 0.0f;
        case WAVE_SINE: return sinf(phase * (float)TWO_PI);
        case WAVE_SQUARE: {
            float sq = (phase < 0.5f) ? 1.0f : -1.0f;
            sq += polyblep(phase, phase_increment);
            sq -= polyblep(fmodf(phase + 0.5f, 1.0f), phase_increment);
            return sq;
        }
        case WAVE_SAW:
            return (2.0f * phase - 1.0f) - polyblep(phase, phase_increment);
        case WAVE_TRIANGLE: {
            float tri = 1.0f - 4.0f * fabsf(phase - 0.5f);
            tri += 4.0f * phase_increment * polyblamp(phase, phase_increment);
            tri -= 4.0f * phase_increment * polyblamp(fmodf(phase + 0.5f, 1.0f), phase_increment);
            return tri;
        }
        //hate having to clamp these, but there's not much of a better option I can find
        case WHITE_NOISE: {
            float s = rand_gaussian(0.2f);
            return s > 1.0f ? 1.0f : s < -1.0f ? -1.0f : s;
        }
        case PINK_NOISE: {
            float s = generate_pink();
            return s > 1.0f ? 1.0f : s < -1.0f ? -1.0f : s;
        }
        default: return 0.0f;
    }
}

//generates one internal-rate sample and runs it through the FIR delay line.
static float next_internal_sample(void) {
    if (freq_steps_remaining > 0) {
        current_frequency *= freq_ratio;
        freq_steps_remaining--;
        if (freq_steps_remaining == 0) current_frequency = target_frequency;
    }

    if (amp_steps_remaining > 0) {
        current_amplitude += amp_increment;
        amp_steps_remaining--;
        if (amp_steps_remaining == 0) {
            if (waveform_pending) {
                waveform = next_waveform;
                waveform_pending = 0;
                current_amplitude = 0.0f;
                start_amp_ramp(0.0f, target_amplitude);
            } else {
                current_amplitude = target_amplitude;
            }
        }
    }

    float phase = (float)get_phase();
    float phase_increment = current_frequency / (float)internal_sample_rate;
    sample_counter++;

    float sample = generate_wave_sample(phase, phase_increment) * current_amplitude;

    //FIR filter applied to all waveforms except noise
    switch (waveform) {
        case WHITE_NOISE:
        case PINK_NOISE:
            break;
        default:
            sample = fir_process(sample);
            break;
    }

    if (fabsf(sample) < 1e-20f) sample = 0.0f;
    return sample;
}

//entry point called by the SDL audio callback.
//generates OVERSAMPLE_FACTOR internal samples per output sample,
//running each through the FIR filter, then returns the last one.
//point decimation is valid because the FIR has already removed
//everything above output Nyquist to >83dB below signal level.
float signal_next_sample(void) {
    float sample = 0.0f;
    for (int i = 0; i < OVERSAMPLE_FACTOR; i++) {
        sample = next_internal_sample();
    }
    return sample;
}
