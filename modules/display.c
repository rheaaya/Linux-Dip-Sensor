#include "display.h"
#include "sampler.h"
#include "dip_detector.h"
#include "../src/hal/rotary_encoder.h"
#include "periodTimer.h"
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

static pthread_t th;
static volatile int run=0;

static void print_ten(const double *x, int n)
{
    if (n<=0){ printf("  (no samples)\n"); return; }
    int show = n<10? n:10;
    for (int i=0;i<show;i++){
        int idx = (int)lround((double)i*(n-1)/((show-1)?(show-1):1));
        printf("%4d:%.3f  ", idx, x[idx]);
    }
    printf("\n");
}

static void* worker(void*arg)
{
    (void)arg;
    while (run){
        sleep(1);

        sampler_roll_history();

        Period_statistics_t s={0};
        Period_getStatisticsAndClear(PERIOD_EVENT_SAMPLE_LIGHT, &s);
        sampler_set_last_second_sample_count(s.numSamples);

        int n=0; double *hist = sampler_get_history_copy(&n);
        double avg = sampler_get_avg_volts();
        int dips = dip_count(hist, n, avg);
        sampler_set_last_second_dips(dips);

        printf("#Smpl/s = %-4d  ", s.numSamples);
        printf("Flash @ %3dHz   ", re_get_frequency_hz());
        printf("avg = %.3fV   ", avg);
        printf("dips = %3d   ", dips);
        printf("Smpl ms[%6.3f, %6.3f] avg %6.3f/%d\n",
               s.minPeriodInMs, s.maxPeriodInMs, s.avgPeriodInMs, s.numSamples);

        print_ten(hist, n);
        free(hist);
        fflush(stdout);
    }
    return NULL;
}

void display_start(void){ run=1; pthread_create(&th,NULL,worker,NULL); }
void display_stop(void){ run=0; pthread_join(th,NULL); }
