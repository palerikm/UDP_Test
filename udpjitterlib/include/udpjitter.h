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

struct TestRunPktResponse{
    uint32_t pktCookie;
    uint32_t seq;
    int64_t jitter_ns;
    int64_t txInterval_ns;
};

struct TestPacket{
    uint32_t pktCookie;
    uint32_t seq;
    uint32_t cmd;
    int64_t txInterval;
    int32_t lastSeqConfirmed;
};


struct TestData{
    struct TestPacket pkt;
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
    //void (* TestRun_live_cb)(int, uint32_t seq, int64_t);
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

    //struct TestRunManager *mngr;
    struct TestRunStatistics stats;
    int32_t lastSeqConfirmed;
    pthread_mutex_t lock;
    FILE *fptr;

    void (* TestRun_live_cb)(int, uint32_t seq, int64_t);
    bool done;
};



//void initTestRunManager(struct TestRunManager *testRunManager);
struct TestRunManager* newTestRunManager();
void freeTestRunManager(struct TestRunManager **mngr);


int initTestRun(struct TestRun *testRun, int32_t id, const struct FiveTuple *fiveTuple,
        struct TestRunConfig *config, void (*liveCb)(int, uint32_t seq, int64_t), bool liveUpdate);
int freeTestRun(struct TestRun *testRun);
void addTestRun(struct TestRunManager *mng, struct TestRun *tRun);
struct TestRun* findTestRun(struct TestRunManager *mng, struct FiveTuple *fiveTuple);
bool saveAndDeleteFinishedTestRuns(struct TestRunManager *mngr, const char *filename);
bool pruneLingeringTestRuns(struct TestRunManager *mngr);


//Make these available for tests
int addTestData(struct TestRun *testRun, const struct TestPacket *testPacket, int pktSize, const struct timespec *now);
int insertResponseData(uint8_t *buf, size_t bufsize, struct TestRun *run );

double getActiveBwOnAllTestRuns(struct TestRunManager *mngr);
int getNumberOfActiveTestRuns(struct TestRunManager *mngr);
uint32_t getPktLossOnAllTestRuns(struct TestRunManager *mngr);

int addTestDataFromBuf(struct TestRunManager *mng, struct FiveTuple *fiveTuple, const unsigned char* buf, int buflen, const struct timespec *now);


struct TestPacket getEndTestPacket(int num);
struct TestPacket getStartTestPacket();
uint32_t fillPacket(struct TestPacket *testPacket, uint32_t seq, uint32_t cmd, int64_t tDiff, int32_t lastSeqConfirmed);


int insertResponseData(uint8_t *buf, size_t bufsize, struct TestRun *run );
int extractRespTestData(const unsigned char *buf, struct TestRun *run);

#ifdef __cplusplus
}
#endif
#endif //UDP_TESTS_UDPJITTER_H
