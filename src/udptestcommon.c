//
// Created by Pal-Erik Martinsen on 10/12/2020.
//


#include <time.h>
#include "udptestcommon.h"


void timeStartTest(struct TimingInfo *tInfo){
    clock_gettime(CLOCK_MONOTONIC_RAW, &tInfo->startBurst);
    tInfo->startOfTest = tInfo->startBurst;
}

void timeSendPacket(struct TimingInfo *tInfo){
    clock_gettime(CLOCK_MONOTONIC_RAW, &tInfo->timeBeforeSendPacket);
    timespec_sub(&tInfo->timeSinceLastPkt, &tInfo->timeBeforeSendPacket, &tInfo->timeLastPacket);
}

void timeStartBurst(struct TimingInfo *tInfo){
    clock_gettime(CLOCK_MONOTONIC_RAW, &tInfo->startBurst);
    tInfo->timeLastPacket = tInfo->timeBeforeSendPacket;
}

void sleepBeforeNextBurst(struct TimingInfo *tInfo, int pktsInBurst, int *currBurstIdx, const struct timespec *pktDelay){
    struct timespec delay = getBurstDelay(pktsInBurst, currBurstIdx, tInfo, pktDelay);
    nap(&delay, &tInfo->overshoot);
}


int nap(const struct timespec *naptime, struct timespec *overshoot){
    struct timespec ts, te, td = {0,0};
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    nanosleep(naptime, NULL);
    clock_gettime(CLOCK_MONOTONIC_RAW, &te);
    timespec_sub(&td, &te, &ts);
    timespec_sub(overshoot, &td, naptime);
    return 0;
}

struct timespec getBurstDelay(int pktsInBurst, int *currBurstIdx, struct TimingInfo *tInfo, const struct timespec *pktDelay) {
    struct timespec delay = {0, 0};
    if (*currBurstIdx < pktsInBurst) {
        (*currBurstIdx)++;
        return delay;
    } else {
        *currBurstIdx = 0;
        //Time when burst ended
        clock_gettime(CLOCK_MONOTONIC_RAW, &tInfo->endBurst);
        //How long did we burst?
        timespec_sub(&tInfo->inBurst, &tInfo->endBurst, &tInfo->startBurst);
        timespec_sub(&delay, pktDelay, &tInfo->inBurst );
        timespec_sub(&delay, &delay, &tInfo->overshoot);
        return delay;
    }
}
