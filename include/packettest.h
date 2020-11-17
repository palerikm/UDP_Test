//
// Created by Pal-Erik Martinsen on 12/11/2020.
//

#ifndef UDP_TESTS_PACKETTEST_H
#define UDP_TESTS_PACKETTEST_H

#define TEST_PKT_COOKIE 0x0023

#include <stdint.h>
#include <time.h>
#include <stdbool.h>
//#include <pthread.h>

struct TestPacket{
    uint32_t pktCookie;
    uint32_t srcId;
    uint32_t seq;
};

struct TestData{
    struct TestPacket pkt;
    long timeDiff;
    uint32_t lostPkts;
};

struct TestRunConfig{
    int numPktsToSend;
    int delayns;
};

struct TestRun{
    struct TestRunConfig config;
    struct TestData *testData;
    uint32_t numTestData;
    uint32_t maxNumTestData;

    struct timespec lastPktTime;
    bool done;
};

int initTestRun(struct TestRun *testRun, uint32_t maxNumPkts,
        struct TestRunConfig config);

int freeTestRun(struct TestRun *testRun);

int addTestData(struct TestRun *testRun, struct TestPacket *testPacket);
int addTestDataFromBuf(struct TestRun *testRun, const unsigned char* buf, int buflen);
struct TestPacket getNextTestPacket(const struct TestRun *testRun);
struct TestPacket getEndTestPacket(const struct TestRun *testRun);

uint32_t fillPacket(struct TestPacket *testPacket, uint32_t srcId, uint32_t seq);

void saveTestDataToFile(const struct TestRun *testRun, const char* filename);

static inline void timespec_diff(struct timespec *a, struct timespec *b,
                                 struct timespec *result) {
    result->tv_sec  = a->tv_sec  - b->tv_sec;
    result->tv_nsec = a->tv_nsec - b->tv_nsec;
    if (result->tv_nsec < 0) {
        --result->tv_sec;
        result->tv_nsec += 1000000000L;
    }
}
#endif //UDP_TESTS_PACKETTEST_H
