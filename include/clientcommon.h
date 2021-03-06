//
// Created by Pal-Erik Martinsen on 05/01/2021.
//

#ifndef UDP_TESTS_CLIENTCOMMON_H
#define UDP_TESTS_CLIENTCOMMON_H
#ifdef __cplusplus
extern "C"
{
#endif


#include "../udpjitterlib/include/udpjitter.h"
#include "../sockethelperlib/include/sockethelper.h"
#include "../udpjitterlib/include/timing.h"




struct TestRun* addRxTestRun(const struct TestRunConfig *testRunConfig,
                             struct TestRunManager *testRunManager,
                             struct ListenConfig *listenConfig,
                             void (*jitterCb)(int, uint32_t seq, int64_t),
                             void (*pktLossCb)(int, uint32_t, uint32_t),
                             bool liveCSV);

struct TestRun* addTxTestRun(const struct TestRunConfig *testRunConfig,
                             struct TestRunManager *testRunManager,
                             struct ListenConfig *listenConfig,
                             void (*liveCb)(int, uint32_t seq, int64_t),
                             void (*pktLossCb)(int, uint32_t, uint32_t),
                             bool liveCSV);

void addTxAndRxTests(struct TestRunConfig *testRunConfig,
                     struct TestRunManager *testRunManager,
                     struct ListenConfig *listenConfig,
                     void (*liveCb)(int, uint32_t seq, int64_t),
                     void (*pktLossCb)(int, uint32_t, uint32_t),
                     bool liveCSV);

int runAllTests(int sockfd, struct TestRunConfig *testRunConfig, struct TestRunManager *testRunManager, struct ListenConfig *listenConfig);

void waitForResponses(struct TestRunConfig *testRunConfig, struct TestRunManager *testRunManager);

void
packetHandler(struct ListenConfig* config,
              struct sockaddr*     from_addr,
              void*                cb,
              unsigned char*       buf,
              int                  buflen);

int
setupSocket(struct ListenConfig *config);

int startListenThread(struct TestRunManager *testRunManager, struct ListenConfig* listenConfig);

#ifdef __cplusplus
}
#endif

#endif //UDP_TESTS_CLIENTCOMMON_H
