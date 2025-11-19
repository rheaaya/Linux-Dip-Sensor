// dip_detector.h - Light dip detection using hysteresis thresholds

#ifndef MODULE_DIP_DETECTOR_H
#define MODULE_DIP_DETECTOR_H
int dip_count(const double *x, int n, double avg_volts);
// Hysteresis: trigger at avg-0.10V, re-arm at avg-0.07V
#endif
