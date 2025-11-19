// led.c - Software PWM implementation for LED control
// Uses precise timing to generate square waves at desired frequencies

#include "led.h"
#include <gpiod.h>
#include <pthread.h>
#include <stdatomic.h>
#include <time.h>
#include <stdio.h>

// Your existing working code remains exactly the same...
// [ALL YOUR EXISTING CODE HERE - DON'T CHANGE IT]

static struct gpiod_chip *chip = NULL;
static struct gpiod_line_settings *out = NULL;
static struct gpiod_line_config *lcfg = NULL;
static struct gpiod_request_config *rcfg = NULL;
static struct gpiod_line_request *req = NULL;
static unsigned offset = 0;

static atomic_int s_hz = 0;
static volatile int s_run = 0;
static pthread_t s_thr;

static void sleep_ns(long ns)
{
    struct timespec ts = { ns / 1000000000L, ns % 1000000000L };
    nanosleep(&ts, NULL);
}

static void* blink_thread(void *arg)
{
    (void)arg;
    while (s_run) {
        int hz = s_hz;
        if (hz <= 0) {
            // LED off when frequency is 0
            gpiod_line_request_set_value(req, offset, GPIOD_LINE_VALUE_INACTIVE);
            sleep_ns(50 * 1000 * 1000); // 50 ms idle
            continue;
        }

        // Square wave at requested frequency
        long half_ns = 1000000000L / (2 * hz);
        gpiod_line_request_set_value(req, offset, GPIOD_LINE_VALUE_ACTIVE);
        sleep_ns(half_ns);
        gpiod_line_request_set_value(req, offset, GPIOD_LINE_VALUE_INACTIVE);
        sleep_ns(half_ns);
    }
    return NULL;
}

int led_init(const char *chip_path, unsigned led_offset)
{
    offset = led_offset;
    chip = gpiod_chip_open(chip_path);
    if (!chip) { perror("led gpiod_chip_open"); return -1; }

    out = gpiod_line_settings_new();
    gpiod_line_settings_set_direction(out, GPIOD_LINE_DIRECTION_OUTPUT);
    gpiod_line_settings_set_output_value(out, GPIOD_LINE_VALUE_INACTIVE);

    lcfg = gpiod_line_config_new();
    if (gpiod_line_config_add_line_settings(lcfg, &offset, 1, out) < 0) {
        perror("led line_config_add"); return -1;
    }

    rcfg = gpiod_request_config_new();
    gpiod_request_config_set_consumer(rcfg, "led_blinker");

    req = gpiod_chip_request_lines(chip, rcfg, lcfg);
    if (!req) { perror("led chip_request_lines"); return -1; }

    s_hz = 0;
    s_run = 1;
    pthread_create(&s_thr, NULL, blink_thread, NULL);
    return 0;
}

void led_set_frequency(int hz)
{
    if (hz < 0) hz = 0;
    if (hz > 500) hz = 500;
    s_hz = hz;
}

int led_get_frequency(void)
{
    return s_hz;
}

void led_on(void)
{
    s_hz = 0; // disable blinking
    gpiod_line_request_set_value(req, offset, GPIOD_LINE_VALUE_ACTIVE);
}

void led_off(void)
{
    s_hz = 0; // disable blinking
    gpiod_line_request_set_value(req, offset, GPIOD_LINE_VALUE_INACTIVE);
}

void led_cleanup(void)
{
    s_run = 0;
    if (req) pthread_join(s_thr, NULL);

    if (req)  gpiod_line_request_release(req);
    if (rcfg) gpiod_request_config_free(rcfg);
    if (lcfg) gpiod_line_config_free(lcfg);
    if (out)  gpiod_line_settings_free(out);
    if (chip) gpiod_chip_close(chip);

    chip = NULL; out = NULL; lcfg = NULL; rcfg = NULL; req = NULL;
    offset = 0; s_hz = 0;
}
