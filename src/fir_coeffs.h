#ifndef FIR_COEFFS_H
#define FIR_COEFFS_H

// Windowed sinc FIR anti-aliasing filter for 4:1 decimation.
// Designed with a Blackman window, 256 taps, -3dB point at 20kHz,
// internal sample rate 192kHz.
// Frequency response:
//    1kHz:   0.000 dB
//   10kHz:   0.000 dB
//   15kHz:   0.000 dB
//   17kHz:   0.001 dB
//   18kHz:  -0.003 dB
//   19kHz:  -0.669 dB
//   20kHz:  -6.021 dB (-3dB point)
//   21kHz: -22.605 dB
//   22kHz: -68.652 dB
//   24kHz: -83.743 dB (output Nyquist)
// Linear phase -- no phase distortion anywhere in the passband.

#define FIR_TAPS 256

extern const float fir_coeffs[FIR_TAPS];

#endif
