#ifndef SIGNAL_H
#define SIGNAL_H

typedef enum {
	WAVE_SINE,
	WAVE_SQUARE,
	WAVE_SAW,
	WAVE_TRIANGLE
} WaveType;

void signal_init(int sample_rate);
void signal_set_frequency(float freq);
void signal_set_amplitude(float amp);
void signal_set_waveform(WaveType type);
int signal_get_write_index();

float signal_next_sample(void);

const float *signal_get_buffer(void);
int signal_get_buffer_size(void);

#endif

