// rotary_encoder.h - Hardware abstraction for rotary encoder
// Provides frequency control via quadrature decoding

#ifndef HAL_ROTARY_ENCODER_H
#define HAL_ROTARY_ENCODER_H

#define ENCODER_START_FREQ_HZ 10
#define ENCODER_MAX_FREQ_HZ 500
#define ENCODER_MIN_FREQ_HZ 0

// Init with gpiochip path and offsets for A/B (e.g., "/dev/gpiochip2", 8, 7)
int  re_init(const char *chip_path, unsigned offA, unsigned offB);
void re_cleanup(void);

// Frequency control (Hz), clamped [0..500], starts at 10 Hz per rubric
int  re_get_frequency_hz(void);
void re_set_frequency_hz(int hz);

#endif
