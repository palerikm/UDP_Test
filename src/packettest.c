//
// Created by Pal-Erik Martinsen on 12/11/2020.
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "packettest.h"


uint32_t fillPacket(struct TestPacket *testPacket, uint32_t srcId, uint32_t seq){
    testPacket->pktCookie = TEST_PKT_COOKIE;
    testPacket->srcId = srcId;
    testPacket->seq = seq;
    return 0;
}

int initTestRun(struct TestRun *testRun, uint32_t maxNumPkts, const struct TestRunConfig config){
    testRun->testData = malloc(sizeof(struct TestData)*maxNumPkts);
    if(testRun->testData == NULL){
        perror("Error allocating memory for testdata: ");
        return -1;
    }
    testRun->numTestData = 0;
    testRun->maxNumTestData = maxNumPkts;
    testRun->config = config;
    testRun->done = false;
    return 0;
}

int addTestData(struct TestRun *testRun, struct TestPacket *testPacket){
    if(testRun->numTestData >= testRun->maxNumTestData){
        return -2;
    }
    struct timespec now, timeSinceLastPkt;

    if(testRun == NULL){
        return -1;
    }
    clock_gettime(CLOCK_MONOTONIC_RAW, &now);

    //Last packet
    if(testPacket->seq == UINT32_MAX){
        if(testRun->numTestData== 0){
            return 0;
        }

        testRun->done = true;
        return 0;
    }
    //First packet.
    if(testRun->numTestData == 0){
        timeSinceLastPkt.tv_nsec = 0;
    }else {
        timespec_diff(&now, &testRun->lastPktTime, &timeSinceLastPkt);
    }

    struct TestData d;
    d.pkt = *testPacket;
    d.timeDiff = timeSinceLastPkt.tv_nsec;
    d.lostPkts = 0;

    memcpy((testRun->testData+testRun->numTestData), &d, sizeof(struct TestData));
    testRun->numTestData++;

    testRun->lastPktTime = now;
    return 0;
}


int addTestDataFromBuf(struct TestRun *testRun, const unsigned char* buf, int buflen){
    struct TestPacket *pkt;
    if (buflen<sizeof(struct TestPacket)){
        return 1;
    }
    //memcpy(&pkt, buf, sizeof(pkt));
    pkt = (struct TestPacket *)buf;
    if(pkt->pktCookie != TEST_PKT_COOKIE){
        printf("Not a test pkt! Ignoring\n");
        return 1;
    }
    return addTestData(testRun, pkt);
}

struct TestPacket getNextTestPacket(const struct TestRun *testRun){
    struct TestPacket pkt;
    fillPacket(&pkt, 23, testRun->numTestData);
    return pkt;
}

struct TestPacket getEndTestPacket(const struct TestRun *testRun){
    struct TestPacket pkt;
    fillPacket(&pkt, 23, UINT32_MAX);
    return pkt;
}

int freeTestRun(struct TestRun *testRun){
    if(testRun != NULL){
        if(testRun->testData != NULL) {
            free(testRun->testData);
        }
    }
    return 0;
}

void saveTestDataToFile(const struct TestRun *testRun, const char* filename) {
    printf("Saving--- (%i)\n", testRun->numTestData);
    FILE *fptr;
    fptr = fopen(filename,"w");
    if(fptr == NULL)
    {
        printf("Error!");
        exit(1);
    }
    fprintf(fptr, "(Packets:  %i, Delay(ns): %i)\n",
            testRun->config.numPktsToSend, testRun->config.delayns);

    for(int i=0; i<testRun->numTestData;i++){
        const struct TestData *muh;
        muh = testRun->testData+i;
        fprintf(fptr, "%i,%ld\n", muh->pkt.seq, muh->timeDiff);
    }
    fclose(fptr);
    printf("    ----Done saving\n");
}