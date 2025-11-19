// sampler.h - Background thread for continuous light sampling and history management

#ifndef MODULE_SAMPLER_H
#define MODULE_SAMPLER_H
#include <stdbool.h>

void sampler_init(void);     // starts 1 kHz sampling thread
void sampler_cleanup(void);

void sampler_roll_history(void);   // move current->history (called at 1 Hz)

int      sampler_get_history_size(void);
double*  sampler_get_history_copy(int *nOut);   // caller free()
long long sampler_get_total_samples(void);
double   sampler_get_avg_volts(void);

void sampler_set_last_second_dips(int d);
int  sampler_get_last_second_dips(void);

void sampler_set_last_second_sample_count(int n);
int  sampler_get_last_second_sample_count(void);

#endif
