#ifndef SIGNAL_H
#define SIGNAL_H

#include <SDL2/SDL_audio.h>

typedef enum {
	NONE,
	WAVE_SINE,
	WAVE_SQUARE,
	WAVE_SAW,
	WAVE_TRIANGLE,
	WHITE_NOISE,
	PINK_NOISE
} WaveType;

void signal_init(int sample_rate, int channels, SDL_AudioDeviceID d);

float signal_next_sample(void);

void signal_set_frequency(float freq);
void signal_set_amplitude(float amp);
void signal_set_waveform(WaveType type);

float signal_get_frequency();
float signal_get_amplitude();
const char* signal_get_waveform();
int signal_get_channels();
SDL_AudioDeviceID signal_get_device();

#endif
