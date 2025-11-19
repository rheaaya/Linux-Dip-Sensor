// led.h - Software PWM LED control module
// Provides frequency-based LED flashing using software timing

#ifndef LED_H
#define LED_H

#define LED_MAX_FREQ_HZ 500
#define LED_MIN_FREQ_HZ 0
#define LED_POLLING_INTERVAL_MS 20

// Initialize a single LED GPIO line and start a software blinker thread.
// chip_path: e.g. "/dev/gpiochip2"
// led_offset: e.g. 16 for GPIO12 (pin 32 on your header)
int  led_init(const char *chip_path, unsigned led_offset);

// Set/get the blink frequency in Hz (0..500). 0 means LED held OFF.
void led_set_frequency(int hz);
int  led_get_frequency(void);

// Manually control (rarely needed if you're using frequency mode)
void led_on(void);
void led_off(void);

// Stop thread and free resources.
void led_cleanup(void);

#endif
