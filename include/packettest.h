//
// Created by Pal-Erik Martinsen on 12/11/2020.
//

#ifndef UDP_TESTS_PACKETTEST_H
#define UDP_TESTS_PACKETTEST_H

#define TEST_PKT_COOKIE 0x0023
#define MAX_TESTNAME_LEN 33

#include <stdint.h>
#include <time.h>
#include <stdbool.h>
#include <sys/socket.h>

#include <hashmap.h>

static const uint32_t no_op_cmd = 0;
static const uint32_t start_test_cmd = 1;
static const uint32_t in_progress_test_cmd = 2;
static const uint32_t stop_test_cmd = 3;
static const uint32_t echo_pkt_cmd = 4;



struct TestPacket{
    uint32_t pktCookie;
    uint32_t srcId;
    uint32_t seq;
    uint32_t cmd;
    char testName[MAX_TESTNAME_LEN];
};

struct FiveTuple{
    struct sockaddr_storage localAddr;
    struct sockaddr_storage remoteAddr;
    int                     port;
};

struct TestData{
    struct TestPacket pkt;
    struct FiveTuple fiveTuple;
    struct timespec timeDiff;
};

struct TestRunConfig{
    char testName[MAX_TESTNAME_LEN];
    char                    interface[10];
    struct FiveTuple        fiveTuple;
    int numPktsToSend;
    struct timespec delay;
    int pktsInBurst;
    int looseNthPkt;
    int dscp;
    int pkt_size;
};

struct TestRunStatistics{
    uint32_t lostPkts;
    struct timespec startTest;
    struct timespec endTest;
    uint32_t rcvdPkts;
    uint32_t rcvdBytes;
};

struct TestRun{
    struct TestRunConfig config;
    struct TestData *testData;
    uint32_t numTestData;
    uint32_t maxNumTestData;
    struct timespec lastPktTime;

    struct TestRunStatistics stats;
    bool done;
};

struct TestRunManager{
    struct hashmap *map;
    //struct sockaddr_storage localAddr;
    struct TestRunConfig defaultConfig;
};


uint64_t TestRun_hash(const void *item, uint64_t seed0, uint64_t seed1);
int TestRun_compare(const void *a, const void *b, void *udata);
bool TestRun_iter(const void *item, void *udata);
bool TestRun_bw_iter(const void *item, void *udata);

int initTestRun(struct TestRun *testRun,
                 char *name,
                 uint32_t maxNumPkts,
                struct TestRunConfig *config);
int testRunReset(struct TestRun *testRun);
int freeTestRun(struct TestRun *testRun);

int addTestData(struct TestRun *testRun, const struct TestPacket *testPacket, int pktSize, const struct timespec *now);
int addTestDataFromBuf(struct TestRunManager *mng, const struct sockaddr* from_addr, const unsigned char* buf, int buflen, const struct timespec *now);
struct TestPacket getNextTestPacket(const struct TestRun *testRun);
struct TestPacket getEndTestPacket(const struct TestRun *testRun);
struct TestPacket getStartTestPacket(const struct TestRun *testRun);
uint32_t fillPacket(struct TestPacket *testPacket, uint32_t srcId, uint32_t seq, uint32_t cmd, char* testName);

void saveTestDataToFile(const struct TestRun *testRun, const char* filename);

static inline void timespec_diff(const struct timespec *a, const struct timespec *b,
                                 struct timespec *result) {
    result->tv_sec  = a->tv_sec  - b->tv_sec;
    result->tv_nsec = a->tv_nsec - b->tv_nsec;
    if (result->tv_nsec < 0) {
        --result->tv_sec;
        result->tv_nsec += 1000000000L;
    }
}
#endif //UDP_TESTS_PACKETTEST_H
