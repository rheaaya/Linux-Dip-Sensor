#include "rotary_encoder.h"
#include <gpiod.h>
#include <pthread.h>
#include <stdatomic.h>
#include <unistd.h>
#include <stdio.h>

static struct gpiod_chip *s_chip = NULL;
static struct gpiod_line_request *s_req = NULL;
static struct gpiod_line_settings *s_in = NULL;
static struct gpiod_line_config *s_lcfg = NULL;
static struct gpiod_request_config *s_rcfg = NULL;
static struct gpiod_edge_event_buffer *s_buf = NULL;

static unsigned s_offA = 0, s_offB = 0;
static atomic_int s_freqHz;            // shared frequency (0..500)
static volatile int s_run = 0;
static pthread_t s_thr;

static int get_level(unsigned off)
{
    int v = gpiod_line_request_get_value(s_req, off);
    return (v == GPIOD_LINE_VALUE_ACTIVE) ? 1 : 0;
}

static void* thr_fn(void *arg)
{
    (void)arg;
    int a = get_level(s_offA);
    int b = get_level(s_offB);
    int prev = (a << 1) | b;

    while (s_run) {
        int rv = gpiod_line_request_wait_edge_events(s_req, 5000000); // 5s
        if (rv < 0) { perror("rotary wait"); break; }
        if (rv == 0) continue;

        int n = gpiod_line_request_read_edge_events(s_req, s_buf, 16);
        if (n < 0) { perror("rotary read"); break; }

        for (int i = 0; i < n; i++) {
            struct gpiod_edge_event *ev =
                (struct gpiod_edge_event*)gpiod_edge_event_buffer_get_event(s_buf, i);
            unsigned off = gpiod_edge_event_get_line_offset(ev);
            if (off == s_offA) a = get_level(s_offA);
            else if (off == s_offB) b = get_level(s_offB);

            int cur = (a << 1) | b;
            int delta = 0;
            // +1 sequence
            if ((prev==0 && cur==1) || (prev==1 && cur==3) ||
                (prev==3 && cur==2) || (prev==2 && cur==0)) delta = +1;
            // -1 sequence
            else if ((prev==0 && cur==2) || (prev==2 && cur==3) ||
                     (prev==3 && cur==1) || (prev==1 && cur==0)) delta = -1;

            if (delta != 0) {
                int f = s_freqHz + delta;
                if (f < 0) f = 0;
                if (f > 500) f = 500;
                s_freqHz = f;
            }
            prev = cur;
        }
    }
    return NULL;
}

int re_init(const char *chip_path, unsigned offA, unsigned offB)
{
    s_offA = offA; s_offB = offB;
    s_chip = gpiod_chip_open(chip_path);
    if (!s_chip) { perror("gpiod_chip_open"); return -1; }

    s_in = gpiod_line_settings_new();
    gpiod_line_settings_set_direction(s_in, GPIOD_LINE_DIRECTION_INPUT);
    gpiod_line_settings_set_bias(s_in, GPIOD_LINE_BIAS_PULL_UP);
    gpiod_line_settings_set_edge_detection(s_in, GPIOD_LINE_EDGE_BOTH);

    s_lcfg = gpiod_line_config_new();
    const unsigned offs[2] = { s_offA, s_offB };
    if (gpiod_line_config_add_line_settings(s_lcfg, offs, 2, s_in) < 0) {
        perror("line_config_add"); return -1;
    }

    s_rcfg = gpiod_request_config_new();
    gpiod_request_config_set_consumer(s_rcfg, "rotary_encoder");
    gpiod_request_config_set_event_buffer_size(s_rcfg, 16);

    s_req = gpiod_chip_request_lines(s_chip, s_rcfg, s_lcfg);
    if (!s_req) { perror("chip_request_lines"); return -1; }

    s_buf = gpiod_edge_event_buffer_new(16);
    if (!s_buf) { perror("edge_event_buffer"); return -1; }

    s_freqHz = 10;    // rubric: start at 10 Hz
    s_run = 1;
    pthread_create(&s_thr, NULL, thr_fn, NULL);
    return 0;
}

void re_cleanup(void)
{
    s_run = 0;
    if (s_req) { pthread_join(s_thr, NULL); }

    if (s_buf)  gpiod_edge_event_buffer_free(s_buf);
    if (s_req)  gpiod_line_request_release(s_req);
    if (s_rcfg) gpiod_request_config_free(s_rcfg);
    if (s_lcfg) gpiod_line_config_free(s_lcfg);
    if (s_in)   gpiod_line_settings_free(s_in);
    if (s_chip) gpiod_chip_close(s_chip);

    s_buf  = NULL;
    s_req  = NULL;
    s_rcfg = NULL;
    s_lcfg = NULL;
    s_in   = NULL;
    s_chip = NULL;
}

int  re_get_frequency_hz(void) { return s_freqHz; }
void re_set_frequency_hz(int hz) { if (hz < 0) hz = 0; if (hz > 500) hz = 500; s_freqHz = hz; }

