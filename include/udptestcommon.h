//
// Created by Pal-Erik Martinsen on 10/12/2020.
//

#ifndef UDP_TESTS_UDPTESTCOMMON_H
#define UDP_TESTS_UDPTESTCOMMON_H

#define NSEC_PER_SEC 1000000000

int nap(const struct timespec *naptime, struct timespec *overshoot);

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

#endif //UDP_TESTS_UDPTESTCOMMON_H
