// main.c - Light Dip Detector main application
// Coordinates all modules for light sampling and analysis

#include "hal/adc.h"
#include "hal/rotary_encoder.h"
#include "hal/led.h"

#include "sampler.h"
#include "udp_server.h"
#include "display.h"

#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

static volatile int keepRunning = 1;
static void onSig(int s) { (void)s; keepRunning = 0; }

static void sleep_ms(long ms)
{
    struct timespec ts = { .tv_sec = ms / 1000, .tv_nsec = (ms % 1000) * 1000000L };
    nanosleep(&ts, NULL);
}

int main(void)
{
    signal(SIGINT, onSig);

    // Init hardware
    if (!adc_init("/dev/spidev0.0")) return 1;            // MCP3208
    if (re_init("/dev/gpiochip2", 8, 7) != 0) return 1;   // Rotary A=8(GPIO17), B=7(GPIO16)
    if (led_init("/dev/gpiochip2", 16) != 0) return 1;    // LED at GPIO12 (offset 16)

    // Start modules
    sampler_init();
    udp_start();
    display_start();

    // Keep LED blink frequency synced with rotary encoder
    int last = -1;
    while (keepRunning) {
        int f = re_get_frequency_hz();            // 0..500
        if (f != last) {
            led_set_frequency(f);                 // LED visibly slower/faster as you turn
            last = f;
        }
        sleep_ms(20);                             // 50 Hz polling is plenty
    }

    // Cleanup on first Ctrl-C or UDP "stop"
    udp_stop();
    display_stop();
    sampler_cleanup();

    led_cleanup();
    re_cleanup();
    adc_cleanup();
    return 0;
}
