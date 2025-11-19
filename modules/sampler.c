#include "sampler.h"
#include "periodTimer.h"
#include "hal/adc.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SAMPLE_US   1000        // 1 kHz target
#define MAX_PER_SEC 2000        // headroom for timing jitter
#define SMOOTH_ALPHA 0.001      // 0.1% update per sample (EMA)

// thread & data
static pthread_t t;
static volatile int run = 0;
static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

static double bufCur[MAX_PER_SEC];
static int    curLen = 0;
static double *bufHist = NULL;
static int     histLen = 0;

static long long total = 0;
static double avg = 0.0;
static int haveFirst = 0;

static int lastDips = 0;
static int lastSampleCount = 0;

static void sleep_us(long us)
{
    struct timespec ts = { .tv_sec = us / 1000000L, .tv_nsec = (us % 1000000L) * 1000L };
    nanosleep(&ts, NULL);
}

static void* thr(void *arg)
{
    (void)arg;
    while (run) {
        unsigned raw = 0; double v = 0.0;
        adc_read_ch(0, &raw, &v);
        Period_markEvent(PERIOD_EVENT_SAMPLE_LIGHT);

        pthread_mutex_lock(&m);
        if (curLen < MAX_PER_SEC) bufCur[curLen++] = v;
        if (!haveFirst) { avg = v; haveFirst = 1; }
        else avg = (1.0 - SMOOTH_ALPHA) * avg + SMOOTH_ALPHA * v;
        total++;
        pthread_mutex_unlock(&m);

        sleep_us(SAMPLE_US);
    }
    return NULL;
}

void sampler_init(void)
{
    Period_init();
    run = 1;
    pthread_create(&t, NULL, thr, NULL);
}

void sampler_cleanup(void)
{
    run = 0;
    pthread_join(t, NULL);
    Period_cleanup();
    free(bufHist); bufHist = NULL; histLen = 0;
}

void sampler_roll_history(void)
{
    pthread_mutex_lock(&m);
    free(bufHist); bufHist = NULL; histLen = curLen;
    if (histLen > 0) {
        bufHist = malloc(sizeof(double) * histLen);
        memcpy(bufHist, bufCur, sizeof(double) * histLen);
    }
    curLen = 0;
    pthread_mutex_unlock(&m);
}

int sampler_get_history_size(void) { return histLen; }

double* sampler_get_history_copy(int *nOut)
{
    pthread_mutex_lock(&m);
    int n = histLen;
    double *out = NULL;
    if (n > 0) {
        out = malloc(sizeof(double) * n);
        memcpy(out, bufHist, sizeof(double) * n);
    }
    pthread_mutex_unlock(&m);
    if (nOut) *nOut = n;
    return out;
}

long long sampler_get_total_samples(void) { return total; }
double sampler_get_avg_volts(void) { return avg; }

void sampler_set_last_second_dips(int d) { lastDips = d; }
int  sampler_get_last_second_dips(void) { return lastDips; }

void sampler_set_last_second_sample_count(int n) { lastSampleCount = n; }
int  sampler_get_last_second_sample_count(void) { return lastSampleCount; }

