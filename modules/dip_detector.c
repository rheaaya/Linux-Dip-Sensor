#include "dip_detector.h"

int dip_count(const double *x, int n, double avg)
{
    if (!x || n<=0) return 0;
    const double trig  = avg - 0.10;
    const double rearm = avg - 0.07;
    int dips=0, state=0;
    for (int i=0;i<n;i++){
        double v = x[i];
        if (state==0){
            if (v <= trig){ dips++; state=1; }
        } else {
            if (v >= rearm) state=0;
        }
    }
    return dips;
}
