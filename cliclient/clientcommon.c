#include <string.h>
#include <pthread.h>

#include "clientcommon.h"


void printStats(struct TestRunManager *mngr, const struct timespec *now, const struct timespec *startOfTest, int numPkts, int numPktsToSend, int pktSize ) {
    if(numPkts % 100 == 0){
        if( getNumberOfActiveTestRuns(mngr)< 2){
            printf("Hmmm, not receiving from the server. Do not feed the black holes on the Internet!\n");
            printf("Did you send to the right server? (Or check your FW settings)\n");
            fflush(stdout);
            exit(1);
        }
        struct timespec elapsed = {0,0};
        timespec_sub(&elapsed, now, startOfTest);
        double sec = (double)elapsed.tv_sec + (double)elapsed.tv_nsec / 1000000000;


        printf("\r(Mbps : %f, p/s: %f ", (((pktSize *numPkts * 8) / sec) / 1000000), (numPkts / sec));
        if(numPktsToSend > 0){
            int doneP = ((double)numPkts / (double)numPktsToSend)*100;
            printf("Progress: %i %%)", doneP);
        }

        fflush(stdout);
    }
}


struct TestRun *
addTxTestRun(const struct TestRunConfig *testRunConfig,
             struct TestRunManager *testRunManager,
             struct ListenConfig *listenConfig,
                     bool liveCSV) {
    struct FiveTuple *txFiveTuple;
    txFiveTuple = makeFiveTuple((struct sockaddr *)&listenConfig->localAddr,
                                (struct sockaddr *)&listenConfig->remoteAddr, listenConfig->port);
    struct TestRun *txTestRun = malloc(sizeof(struct TestRun));
    struct TestRunConfig txConfig;
    memcpy(&txConfig, testRunConfig, sizeof(txConfig));
    strncat(txConfig.testName, "_tx\0", 5);
    initTestRun(txTestRun, testRunManager,1, txFiveTuple, &txConfig, liveCSV);
    clock_gettime(CLOCK_MONOTONIC_RAW, &txTestRun->lastPktTime);
    txTestRun->stats.startTest = txTestRun->lastPktTime;
    addTestRun(testRunManager, txTestRun);
    free(txFiveTuple);

    return txTestRun;
}

struct TestRun *
addRxTestRun(const struct TestRunConfig *testRunConfig,
             struct TestRunManager *testRunManager,
             struct ListenConfig *listenConfig,
                     bool liveCSV){
    struct FiveTuple *rxFiveTuple;
    rxFiveTuple = makeFiveTuple((struct sockaddr *)&listenConfig->remoteAddr,
                                (struct sockaddr *)&listenConfig->localAddr, listenConfig->port);

    struct TestRun *rxTestRun = malloc(sizeof(struct TestRun));
    struct TestRunConfig rxConfig;
    memcpy(&rxConfig, testRunConfig, sizeof(rxConfig));
    strncat(rxConfig.testName, "_rx\0", 5);
    initTestRun(rxTestRun, testRunManager,2, rxFiveTuple, &rxConfig, liveCSV);
    clock_gettime(CLOCK_MONOTONIC_RAW, &rxTestRun->lastPktTime);
    rxTestRun->stats.startTest = rxTestRun->lastPktTime;

    addTestRun(testRunManager, rxTestRun);
    free(rxFiveTuple);
    return rxTestRun;
}


void addTxAndRxTests(struct TestRunConfig *testRunConfig,
                    struct TestRunManager *testRunManager,
                    struct ListenConfig *listenConfig,
                    bool liveCSV) {

    //addTestRun(1, testRunConfig, testRunManager, listenConfig, true);
    //addTestRun(2, testRunConfig, testRunManager, listenConfig, true);
    struct TestRun *txTestRun = addTxTestRun(testRunConfig, testRunManager, listenConfig, liveCSV);
    struct TestRun *rxTestRun = addRxTestRun(testRunConfig, testRunManager, listenConfig, liveCSV);
    free(rxTestRun);
    free(txTestRun);
}

void sendStarOfTest(struct TestRunConfig *cfg, struct FiveTuple *fiveTuple, int sockfd) {
    //Send End of Test a few times...

    struct TestPacket startPkt = getStartTestPacket();

    struct timespec now, remaining;
    uint8_t endBuf[cfg->pktConfig.pkt_size];
    memcpy(endBuf, &startPkt, sizeof(struct TestPacket));
    memcpy(endBuf+sizeof(startPkt), &cfg->pktConfig, sizeof(struct TestRunPktConfig));


    //FIXme!!!!
    struct TestRunConfig tmpCfg;
    memcpy(&tmpCfg, cfg, sizeof(tmpCfg));
    tmpCfg.pktConfig.numPktsToSend = 10000;
    memcpy(endBuf+sizeof(startPkt), &tmpCfg.pktConfig, sizeof(struct TestRunPktConfig));


    for(int j=0;j<10;j++){
        clock_gettime(CLOCK_MONOTONIC_RAW, &now);
        sendPacket(sockfd, (const uint8_t *)&endBuf, sizeof(endBuf),
                   (const struct sockaddr*)&fiveTuple->dst,
                   0, cfg->pktConfig.dscp, 0 );

        nanosleep(&cfg->pktConfig.delay, &remaining);
    }
}

struct TestRunResponse getResponse(const struct TestRun *respRun) {
    struct TestRunResponse r;
    if(respRun->numTestData > 1) {
        r.lastSeqConfirmed = respRun->testData[respRun->numTestData - 1].pkt.seq;
    }else{
        r.lastSeqConfirmed = 0;
    }
    return r;
}


int runTests(int sockfd, struct FiveTuple *txFiveTuple,
             struct TestRunConfig *testRunConfig, struct TestRunManager *testRunManager) {
    int numPkt = 0;
    int numPkts_to_send = testRunConfig->pktConfig.numPktsToSend;
    uint8_t buf[(*testRunConfig).pktConfig.pkt_size];
    memset(&buf, 43, sizeof(buf));

    struct TestPacket pkt;
    int currBurstIdx = 0;



    struct TestRun *respRun = findTestRun(testRunManager, txFiveTuple);
    if(respRun == NULL){
        printf("\nSomething terrible has happened! Cant find the response testrun!\n");
        char ft[200];
        printf("FiveTuple: %s\n", fiveTupleToString(ft, txFiveTuple));
        exit(2);
    }


    struct TimingInfo timingInfo;
    memset(&timingInfo, 0, sizeof(timingInfo));
    clock_gettime(CLOCK_MONOTONIC_RAW, &timingInfo.startBurst);
    timeStartTest(&timingInfo);
    timeStartBurst(&timingInfo);
    while(!testRunManager->done){
        numPkt++;
        struct TestRunResponse r = getResponse(respRun);

        timeSendPacket(&timingInfo);

        //uint32_t seq = numPkt >= numPkts_to_send ? numPkts_to_send : numPkt + 1;

        uint32_t seq = numPkt;
        fillPacket(&pkt, seq, in_progress_test_cmd,
                   &timingInfo.timeSinceLastPkt, &r);
        memcpy(buf, &pkt, sizeof(pkt));

        //printf("Sending packet (%i)\n", seq);

        sendPacket(sockfd, (const uint8_t *) &buf, sizeof(buf),
                   (const struct sockaddr *) &txFiveTuple->dst,
                   0, (*testRunConfig).pktConfig.dscp, 0);

        if(testRunManager->TestRun_status_cb != NULL){
            struct timespec elapsed = {0,0};
            timespec_sub(&elapsed, &timingInfo.timeBeforeSendPacket, &timingInfo.startOfTest);
            double sec = (double)elapsed.tv_sec + (double)elapsed.tv_nsec / 1000000000;
            double mbps = (((double)(testRunConfig->pktConfig.pkt_size *numPkt * 8) / sec)/ 1000000);
            double ps = numPkt / sec;
            testRunManager->TestRun_status_cb(mbps, ps);
            //printf("\r(Mbps : %f, p/s: %f Progress: %i %%)", (((pktSize *numPkts * 8) / sec) / 1000000), numPkts / sec, done);
        }else {
            printStats(testRunManager, &timingInfo.timeBeforeSendPacket, &timingInfo.startOfTest, numPkt,
                       (*testRunConfig).pktConfig.numPktsToSend, (*testRunConfig).pktConfig.pkt_size);
        }
        //Do I sleep or am I bursting..
        sleepBeforeNextBurst(&timingInfo, (*testRunConfig).pktConfig.pktsInBurst, &currBurstIdx, &(*testRunConfig).pktConfig.delay);

        //Staring a new burst when we start at top of for loop.
        timeStartBurst(&timingInfo);

        if(numPkts_to_send > 0 && numPkt > numPkts_to_send){
            if(r.lastSeqConfirmed == (*testRunConfig).pktConfig.numPktsToSend) {
                struct TestRun *txrun = findTestRun(testRunManager, txFiveTuple);
                txrun->done = true;
                testRunManager->done = true;
            }
        }

    }//End of main test run. Just some cleanup remaining.
    return numPkt;
}

void sendEndOfTest(struct TestRunConfig *cfg, struct FiveTuple *fiveTuple, int num, int sockfd) {

    struct TestPacket pkt = getEndTestPacket(num);
    struct timespec now, remaining;
    uint8_t endBuf[cfg->pktConfig.pkt_size];
    memcpy(endBuf, &pkt, sizeof(struct TestPacket));

    for(int j=0;j<10;j++){
        clock_gettime(CLOCK_MONOTONIC_RAW, &now);
        sendPacket(sockfd, (const uint8_t *)&endBuf, sizeof(endBuf),
                   (const struct sockaddr*)&fiveTuple->dst,
                   0, cfg->pktConfig.dscp, 0 );

        nanosleep(&cfg->pktConfig.delay, &remaining);
    }
}

void runAllTests(int sockfd, struct TestRunConfig *testRunConfig, struct TestRunManager *testRunManager,
                 struct ListenConfig *listenConfig) {
    struct FiveTuple *txFiveTuple;
    txFiveTuple = makeFiveTuple((struct sockaddr *)&listenConfig->localAddr,
                                (struct sockaddr *)&listenConfig->remoteAddr, listenConfig->port);

    sendStarOfTest(testRunConfig, txFiveTuple , sockfd);
    int numPkt = runTests(sockfd, txFiveTuple, testRunConfig, testRunManager);
    sendEndOfTest(testRunConfig, txFiveTuple, numPkt, sockfd);
    free(txFiveTuple);
}

void waitForResponses(struct TestRunConfig *testRunConfig, struct TestRunManager *testRunManager) {
    bool done = false;
    char fileEnding[] = "_testrun.txt\0";

    while(!done){
        int num = getNumberOfActiveTestRuns(testRunManager);
        if(  num > 0 ){
            saveAndDeleteFinishedTestRuns(testRunManager, fileEnding);
            struct timespec overshoot = {0,0};
            nap(&(*testRunConfig).pktConfig.delay, &overshoot);
        }else{
            done = true;
        }
    }
}

void
packetHandler(struct ListenConfig* config,
              __unused struct sockaddr*     from_addr,
              __unused void*                cb,
              unsigned char*       buf,
              int                  buflen) {

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC_RAW, &now);
    struct TestRunManager *mngr = config->tInst;
    struct FiveTuple *tuple;
    tuple = makeFiveTuple((const struct sockaddr *)&config->remoteAddr,
                          (const struct sockaddr *)&config->localAddr,
                          config->port);


    int pos = addTestDataFromBuf(mngr, tuple, buf, buflen, &now);


    if(pos<0){
        printf("Encountered error while processing incoming UDP packet\n");
    }
    //lets check if there is more stuff in the packet..
    if(pos > 5) {
        fflush(stdout);
        struct FiveTuple *txtuple = makeFiveTuple((const struct sockaddr *) &config->localAddr,
                                                  (const struct sockaddr *) &config->remoteAddr,
                                                  config->port);
        struct TestRun *run = findTestRun(mngr, txtuple);
        if(run != NULL) {
            extractRespTestData(buf + pos, run);
        }

        free(txtuple);
    }

    free(tuple);

}

int
setupSocket(struct ListenConfig *config)
{
    int sockfd = createLocalSocket(config->remoteAddr.ss_family,
                                   (struct sockaddr*)&config->localAddr,
                                   SOCK_DGRAM,
                                   0);

    config->socketConfig[0].sockfd = sockfd;
    config->numSockets++;

    return config->numSockets;
}

int startListenThread(struct TestRunManager *testRunManager, struct ListenConfig* listenConfig) {

    listenConfig->pkt_handler          = packetHandler;
    listenConfig->tInst = testRunManager;
    pthread_create(&listenConfig->socketListenThread,
                   NULL,
                   socketListenDemux,
                   (void*)listenConfig);

    return listenConfig->socketConfig[0].sockfd;

}

