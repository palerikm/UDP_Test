//
// Created by Pal-Erik Martinsen on 12/11/2020.
//

#ifndef UDP_TESTS_UDPJITTER_H
#define UDP_TESTS_UDPJITTER_H
#ifdef __cplusplus
extern "C"
{
#endif


#define TEST_DATA_INCREMENT_SIZE 10000
#define TEST_PKT_COOKIE 0x0023
#define TEST_RESP_PKT_COOKIE 0x1423
#define MAX_TESTNAME_LEN 33

#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <stdbool.h>
#include <sys/socket.h>

//#include <hashmap.h>

static const uint32_t no_op_cmd = 0;
static const uint32_t start_test_cmd = 1;
static const uint32_t in_progress_test_cmd = 2;
static const uint32_t stop_test_cmd = 3;
static const uint32_t echo_pkt_cmd = 4;
static const uint32_t transport_resp_cmd = 5;




struct TestRunPktConfig{
    int numPktsToSend;
    struct timespec delay;
    int pktsInBurst;
    int dscp;
    int pkt_size;
};

struct TestRunResponse{
    int32_t lastSeqConfirmed;
};

struct TestRunPktResponse{
    uint32_t pktCookie;
    uint32_t seq;
    int64_t txDiff;
    int64_t rxDiff;
};

struct TestPacket{
    uint32_t pktCookie;
    uint32_t seq;
    uint32_t cmd;
    //struct timespec txDiff;
    int64_t txDiff;
    struct TestRunResponse resp;
};

struct FiveTuple{
    struct sockaddr_storage src;
    struct sockaddr_storage dst;
    uint16_t                port;
};

struct TestData{
    struct TestPacket pkt;
    int64_t rxDiff;
    int64_t jitter_ns;
};

struct TestRunConfig{
    char testName[MAX_TESTNAME_LEN];

    struct TestRunPktConfig pktConfig;
};

struct TestRunStatistics{
    uint32_t lostPkts;
    struct timespec startTest;
    struct timespec endTest;
    uint32_t rcvdPkts;
    uint32_t rcvdBytes;
};

struct TestRunManager{
    struct hashmap *map;
    void (* TestRun_live_cb)(int, uint32_t seq, int64_t);
    void (* TestRun_status_cb)(double mbps, double ps);
    bool done;
};

struct TestRun{
    struct FiveTuple *fiveTuple;
    int32_t id;
    struct TestRunConfig config;
    struct TestData *testData;
    uint32_t numTestData;
    uint32_t maxNumTestData;
    struct timespec lastPktTime;

    struct TestRunManager *mngr;

    struct TestRunStatistics stats;
    struct TestRunResponse resp;
    pthread_mutex_t lock;
    FILE *fptr;
    bool done;
};



void initTestRunManager(struct TestRunManager *testRunManager);
uint64_t TestRun_hash(const void *item, uint64_t seed0, uint64_t seed1);
int TestRun_compare(const void *a, const void *b, void *udata);
bool TestRun_iter(const void *item, void *udata);
bool TestRun_bw_iter(const void *item, void *udata);



int freeTestRun(struct TestRun *testRun);
int initTestRun(struct TestRun *testRun, struct TestRunManager *mngr, int32_t id, const struct FiveTuple *fiveTuple, struct TestRunConfig *config, bool liveUpdate);
void addTestRun(struct TestRunManager *mng, struct TestRun *tRun);
int addTestDataFromBuf(struct TestRunManager *mng, struct FiveTuple *fiveTuple, const unsigned char* buf, int buflen, const struct timespec *now);
int addTestData(struct TestRun *testRun, const struct TestPacket *testPacket, int pktSize, const struct timespec *now);
struct TestPacket getNextTestPacket(const struct TestRun *testRun, struct timespec *now);
struct TestPacket getEndTestPacket(int num);
struct TestPacket getStartTestPacket();
uint32_t fillPacket(struct TestPacket *testPacket, uint32_t seq, uint32_t cmd, int64_t tDiff, struct TestRunResponse *resp);


int insertResponseData(uint8_t *buf, size_t bufsize, struct TestRun *run );
int extractRespTestData(const unsigned char *buf, struct TestRun *run);


struct TestRun* findTestRun(struct TestRunManager *mng, struct FiveTuple *fiveTuple);
void saveTestRunToFile(const struct TestRun *testRun, const char* filename);
void saveTestData(const struct TestData *tData, FILE *fptr);


bool saveAndDeleteFinishedTestRuns(struct TestRunManager *mngr, const char *filename);
bool pruneLingeringTestRuns(struct TestRunManager *mngr);
double getActiveBwOnAllTestRuns(struct TestRunManager *mngr);
int getNumberOfActiveTestRuns(struct TestRunManager *mngr);

uint32_t getPktLossOnAllTestRuns(struct TestRunManager *mngr);
void freeTestRunManager(struct TestRunManager *mngr);

struct FiveTuple* makeFiveTuple(const struct sockaddr* from_addr,
              const struct sockaddr* to_addr,
              int port);

char  *fiveTupleToString(char *str, const struct FiveTuple *tuple);

bool TestRun_print_iter(const void *item, void *udata);

#ifdef __cplusplus
}
#endif
#endif //UDP_TESTS_UDPJITTER_H
