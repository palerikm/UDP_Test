//
// Created by Pal-Erik Martinsen on 12/11/2020.
//

#ifndef UDP_TESTS_PACKETTEST_H
#define UDP_TESTS_PACKETTEST_H

#define TEST_PKT_COOKIE 0x0023
#define TEST_RESP_PKT_COOKIE 0x1423
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




struct TestRunPktConfig{
    int numPktsToSend;
    struct timespec delay;
    int pktsInBurst;
    int dscp;
    int pkt_size;
};

struct TestRunResponse{
    int32_t lastIdxConfirmed;
};

struct TestRunPktResponse{
    uint32_t pktCookie;
    uint32_t seq;
    struct timespec txDiff;
    struct timespec rxDiff;
    int64_t jitter_ns;
};

struct TestPacket{
    uint32_t pktCookie;
    uint32_t srcId;
    uint32_t seq;
    uint32_t cmd;
    struct timespec txDiff;
    struct TestRunResponse resp;
    char testName[MAX_TESTNAME_LEN];
};

struct FiveTuple{
    struct sockaddr_storage src;
    struct sockaddr_storage dst;
    uint16_t                port;
};

struct TestData{
    struct TestPacket pkt;
    struct timespec rxDiff;
    int64_t jitter_ns;
};

struct TestRunConfig{
    char testName[MAX_TESTNAME_LEN];
    struct TestRunPktConfig pktConfig;
};

struct JitterInfo{
    int32_t avgJitter;
    int32_t maxJitter;
};


struct TestRunStatistics{
    uint32_t lostPkts;
    struct timespec startTest;
    struct timespec endTest;
    uint32_t rcvdPkts;
    uint32_t rcvdBytes;
    struct JitterInfo jitterInfo;
};


struct TestRun{
    struct FiveTuple *fiveTuple;
    struct TestRunConfig config;
    struct TestData *testData;
    uint32_t numTestData;
    uint32_t maxNumTestData;
    struct timespec lastPktTime;


    struct TestRunStatistics stats;
    struct TestRunResponse resp;
    bool done;
};

struct TestRunManager{
    struct hashmap *map;
};


uint64_t TestRun_hash(const void *item, uint64_t seed0, uint64_t seed1);
int TestRun_compare(const void *a, const void *b, void *udata);
bool TestRun_iter(const void *item, void *udata);
bool TestRun_bw_iter(const void *item, void *udata);



int freeTestRun(struct TestRun *testRun);
int initTestRun(struct TestRun *testRun, uint32_t maxNumPkts, const struct FiveTuple *fiveTuple, struct TestRunConfig *config);
int addTestDataFromBuf(struct TestRunManager *mng, struct FiveTuple *fiveTuple, const unsigned char* buf, int buflen, const struct timespec *now);
int addTestData(struct TestRun *testRun, const struct TestPacket *testPacket, int pktSize, const struct timespec *now);
struct TestPacket getNextTestPacket(const struct TestRun *testRun, struct timespec *now);
struct TestPacket getEndTestPacket(const char *testName, int num);
struct TestPacket getStartTestPacket(const char *testName);
uint32_t fillPacket(struct TestPacket *testPacket, uint32_t srcId, uint32_t seq, uint32_t cmd, struct timespec *tDiff, struct TestRunResponse *resp, const char* testName);
struct TestRun* findTestRun(struct TestRunManager *mng, struct FiveTuple *fiveTuple);
void saveTestDataToFile(const struct TestRun *testRun, const char* filename);

int configToString(char* configStr, const struct TestRunConfig *config);

bool saveAndDeleteFinishedTestRuns(struct TestRunManager *mngr, const char *filename);
bool pruneLingeringTestRuns(struct TestRunManager *mngr);
double getActiveBwOnAllTestRuns(struct TestRunManager *mngr);
int getNumberOfActiveTestRuns(struct TestRunManager *mngr);
int64_t getMaxJitterTestRuns(struct TestRunManager *mngr);
uint32_t getPktLossOnAllTestRuns(struct TestRunManager *mngr);
void freeTestRunManager(struct TestRunManager *mngr);

struct FiveTuple* makeFiveTuple(const struct sockaddr* from_addr,
              const struct sockaddr* to_addr,
              int port);

char  *fiveTupleToString(char *str, const struct FiveTuple *tuple);

bool TestRun_print_iter(const void *item, void *udata);

#endif //UDP_TESTS_PACKETTEST_H
