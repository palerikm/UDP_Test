//
// Created by Pal-Erik Martinsen on 05/01/2021.
//

#ifndef UDP_TESTS_CLIENTCOMMON_H
#define UDP_TESTS_CLIENTCOMMON_H
#include "packettest.h"
#include "sockethelper.h"
#include "udptestcommon.h"




struct TestRun* addRxTestRun(const struct TestRunConfig *testRunConfig,
             struct TestRunManager *testRunManager,
             struct ListenConfig *listenConfig);

struct TestRun* addTxTestRun(const struct TestRunConfig *testRunConfig,
             struct TestRunManager *testRunManager,
             struct ListenConfig *listenConfig);

void addTxAndRxTests(struct TestRunConfig *testRunConfig,
                     struct TestRunManager *testRunManager,
                     struct ListenConfig *listenConfig);

void runAllTests(int sockfd, struct TestRunConfig *testRunConfig, struct TestRunManager *testRunManager, struct ListenConfig *listenConfig);

void waitForResponses(struct TestRunConfig *testRunConfig, struct TestRunManager *testRunManager);

void
packetHandler(struct ListenConfig* config,
              struct sockaddr*     from_addr,
              void*                cb,
              unsigned char*       buf,
              int                  buflen);

int
setupSocket(struct ListenConfig *lconf, const struct ListenConfig* sconfig);

int startListenThread(struct TestRunManager *testRunManager, struct ListenConfig* listenConfig);
#endif //UDP_TESTS_CLIENTCOMMON_H
