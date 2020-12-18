//
// Created by Pal-Erik Martinsen on 10/12/2020.
//


#include <time.h>
#include "udptestcommon.h"

int nap(const struct timespec *naptime, struct timespec *overshoot){
    struct timespec ts, te, td, over = {0,0};
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    nanosleep(naptime, NULL);
    clock_gettime(CLOCK_MONOTONIC_RAW, &te);
    timespec_sub(&td, &te, &ts);
    timespec_sub(overshoot, &td, naptime);
    return 0;
}

struct timespec getBurstDelay(const struct TestRunConfig *cfg, struct timespec *startBurst, struct timespec *endBurst,
                              struct timespec *inBurst, int *currBurstIdx, struct timespec *overshoot) {
    struct timespec delay = {0, 0};
    if (*currBurstIdx < cfg->pktConfig.pktsInBurst) {
        (*currBurstIdx)++;
        return delay;
    } else {
        *currBurstIdx = 0;
        clock_gettime(CLOCK_MONOTONIC_RAW, endBurst);
        timespec_sub(inBurst, endBurst, startBurst);
        timespec_sub(&delay, &cfg->pktConfig.delay, inBurst );
        timespec_sub(&delay, &delay, overshoot);
        return delay;
    }
}
