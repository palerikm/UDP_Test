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

void waitForSomeTime(const struct TestRunConfig *cfg, struct timespec *startBurst, struct timespec *endBurst,
                            struct timespec *inBurst, int currBurstIdx, struct timespec *overshoot) {
    if (currBurstIdx < cfg->pktConfig.pktsInBurst) {
        currBurstIdx++;
    } else {
        currBurstIdx = 0;
        struct timespec delay = {0, 0};
        clock_gettime(CLOCK_MONOTONIC_RAW, endBurst);
        timespec_sub(inBurst, endBurst, startBurst);
        delay.tv_sec = cfg->pktConfig.delay.tv_sec - (*inBurst).tv_sec - (*overshoot).tv_sec;
        delay.tv_nsec = cfg->pktConfig.delay.tv_nsec - (*inBurst).tv_nsec - (*overshoot).tv_nsec;
        nap(&delay, overshoot);
        //Staring a new burst when we start at top of for loop.
        clock_gettime(CLOCK_MONOTONIC_RAW, startBurst);
    }
}
