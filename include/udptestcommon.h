//
// Created by Pal-Erik Martinsen on 10/12/2020.
//

#ifndef UDP_TESTS_UDPTESTCOMMON_H
#define UDP_TESTS_UDPTESTCOMMON_H
#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "packettest.h"

#define NSEC_PER_SEC 1000000000

struct TimingInfo{
    struct timespec startOfTest;
    struct timespec startBurst;
    struct timespec endBurst;
    struct timespec inBurst;
    struct timespec timeBeforeSendPacket;
    struct timespec timeLastPacket;
    struct timespec timeSinceLastPkt;
    struct timespec overshoot;
};

void timeStartTest(struct TimingInfo *tInfo);
void timeSendPacket(struct TimingInfo *tInfo);
void timeStartBurst(struct TimingInfo *tInfo);
void sleepBeforeNextBurst(struct TimingInfo *tInfo, int pktsInBurst, int *currBurstIdx, const struct timespec *pktDelay);

int nap(const struct timespec *naptime, struct timespec *overshoot);

struct timespec getBurstDelay(int pktsInBurst, int *currBurstIdx, struct TimingInfo *tInfo, const struct timespec *pktDelay);

static inline void timespec_sub(struct timespec *result, const struct timespec *a, const struct timespec *b) {
    result->tv_sec  = a->tv_sec  - b->tv_sec;
    result->tv_nsec = a->tv_nsec - b->tv_nsec;
    if (result->tv_nsec < 0) {
        --result->tv_sec;
        result->tv_nsec += NSEC_PER_SEC;
    }
}

static inline int64_t
timespec_to_msec(const struct timespec *a)
{
    return (int64_t)a->tv_sec * 1000 + a->tv_nsec / 1000000;
}

static inline int64_t
timespec_to_nsec(const struct timespec *a)
{
    return (int64_t)a->tv_sec * NSEC_PER_SEC + a->tv_nsec;
}
static inline void
timespec_from_nsec(struct timespec *a, int64_t b)
{
    a->tv_sec = b / NSEC_PER_SEC;
    a->tv_nsec = b % NSEC_PER_SEC;
}

#ifdef __cplusplus
}
#endif
#endif //UDP_TESTS_UDPTESTCOMMON_H
