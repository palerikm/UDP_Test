//
// Created by Pal-Erik Martinsen on 12/11/2020.
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sockethelper.h>

#include "packettest.h"



uint32_t fillPacket(struct TestPacket *testPacket, uint32_t srcId, uint32_t seq, uint32_t cmd){
    testPacket->pktCookie = TEST_PKT_COOKIE;
    testPacket->srcId = srcId;
    testPacket->seq = seq;
    testPacket->cmd = cmd;
    return 0;
}

int initTestRun(struct TestRun *testRun, uint32_t maxNumPkts, struct TestRunConfig *config){
    testRun->testData = malloc(sizeof(struct TestData)*maxNumPkts);
    if(testRun->testData == NULL){
        perror("Error allocating memory for testdata: ");
        return -1;
    }
    testRun->numTestData = 0;
    testRun->maxNumTestData = maxNumPkts;
    testRun->config = config;
    testRun->done = false;
    testRun->stats.lostPkts = 0;
    testRun->stats.rcvdPkts = 0;
    testRun->stats.rcvdBytes =0;
    return 0;
}

int testRunReset(struct TestRun *testRun){
    testRun->done = false;
    testRun->numTestData = 0;
    testRun->lastPktTime.tv_nsec = 0;
    testRun->lastPktTime.tv_sec = 0;
    testRun->stats.rcvdPkts = 0;
    testRun->stats.rcvdBytes = 0;
    testRun->stats.lostPkts = 0;

    return 0;
}

int addTestData(struct TestRun *testRun, struct TestPacket *testPacket, int pktSize){
    struct timespec now, timeSinceLastPkt;

    if(testRun == NULL){
        return -1;
    }
    clock_gettime(CLOCK_MONOTONIC_RAW, &now);

    if(testRun->numTestData >= testRun->maxNumTestData){
        testRun->stats.endTest = now;
        return -2;
    }
    //End of test?
    if(testPacket->cmd == stop_test_cmd){
        testRun->done = true;
        testRun->stats.endTest = now;
        return 0;
    }

    //Start of test?
    if(testPacket->cmd == start_test_cmd){
        testRun->done = false;
        testRun->lastPktTime = now;
        testRun->stats.startTest = now;
        return 0;
    }

    timespec_diff(&now, &testRun->lastPktTime, &timeSinceLastPkt);

    //Did we loose any packets? Or out of order?
    int lostPkts = 0;
    struct TestPacket *lastPkt = &(testRun->testData+testRun->numTestData-1)->pkt;
    if(testPacket->seq < lastPkt->seq ){
        printf("Todo: Fix out of order handling\n");

    }
    if(testPacket->seq > lastPkt->seq+1){
        lostPkts = (testPacket->seq - lastPkt->seq)-1;
        printf("Packet loss (%i)\n", lostPkts);
        for(int i=0; i<lostPkts;i++){
            struct TestPacket tPkt;
            memset(&tPkt, 0, sizeof(tPkt));
            tPkt.seq = lastPkt->seq+1 + i;
            struct TestData d;
            d.pkt = tPkt;
            d.timeDiff.tv_sec = 0;
            d.timeDiff.tv_nsec = 0;
            memcpy((testRun->testData+testRun->numTestData), &d, sizeof(struct TestData));
            testRun->numTestData++;
        }
        testRun->stats.lostPkts+=lostPkts;
    }
    testRun->stats.rcvdPkts++;
    testRun->stats.rcvdBytes+=pktSize;

    struct TestData d;
    d.pkt = *testPacket;
    //Todo: What if over a second delayed....
    d.timeDiff.tv_sec = 0;
    d.timeDiff.tv_nsec = timeSinceLastPkt.tv_nsec;

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
    return addTestData(testRun, pkt, buflen);
}

struct TestPacket getNextTestPacket(const struct TestRun *testRun){
    struct TestPacket pkt;
    fillPacket(&pkt, 23, testRun->numTestData, in_progress_test_cmd);
    return pkt;
}

struct TestPacket getEndTestPacket(const struct TestRun *testRun){
    struct TestPacket pkt;
    fillPacket(&pkt, 23, testRun->numTestData, stop_test_cmd);
    return pkt;
}

struct TestPacket getStartTestPacket(const struct TestRun *testRun){
    struct TestPacket pkt;
    fillPacket(&pkt, 23, testRun->numTestData, start_test_cmd);
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

int configToString(char* configStr, const struct TestRunConfig *config){

    char              addrStr[SOCKADDR_MAX_STRLEN];
    sockaddr_toString( (struct sockaddr*)&config->localAddr,
                               addrStr,
                               sizeof(addrStr),
                               false );
    strncpy(configStr, addrStr, sizeof(addrStr));
    strncat(configStr, " (\0", 4);
    strncat(configStr, config->interface, strlen(config->interface));
    strncat(configStr, ") -> \0", 6);

    sockaddr_toString( (struct sockaddr*)&config->remoteAddr,
                       addrStr,
                       sizeof(addrStr),
                       true );
    strncat(configStr, addrStr, strlen(addrStr));
    char result[50];
    sprintf(result, " PktSize: %i", config->pkt_size);
    strncat(configStr, result, strlen(result));

    sprintf(result, " DSCP: %#x", config->dscp);
    strncat(configStr, result, strlen(result));

    sprintf(result, " Delay: %ld", config->delay.tv_nsec/1000000L);
    strncat(configStr, result, strlen(result));

    return 0;

}

int statsToString(char* configStr, const struct TestRunStatistics *stats) {
    configStr[0] = '\0';
    char result[50];
    sprintf(result, "Pkts: %i", stats->rcvdPkts);
    strncat(configStr, result, strlen(result));

    sprintf(result, " Lost pkts: %i", stats->lostPkts);
    strncat(configStr, result, strlen(result));

    sprintf(result, " Bytes rcvd: %i", stats->rcvdBytes);
    strncat(configStr, result, strlen(result));

    struct timespec elapsed;
    timespec_diff(&stats->endTest, &stats->startTest, &elapsed);

    double sec = (double)elapsed.tv_sec + (double)elapsed.tv_nsec / 1000000000;

    sprintf(result, " Time : %f sec", sec);
    strncat(configStr, result, strlen(result));

    sprintf(result, " (Mbps : %f, p/s: %f) ", (((stats->rcvdBytes*8)/sec)/1000000), stats->rcvdPkts/sec);
    strncat(configStr, result, strlen(result));

    return 0;

}

void saveTestDataToFile(const struct TestRun *testRun, const char* filename) {
    printf("Saving--- %s (%i)\n", filename, testRun->numTestData);
    FILE *fptr;
    fptr = fopen(filename,"w");
    if(fptr == NULL)
    {
        printf("Error!");
        exit(1);
    }
    //fprintf(fptr, "(Packets:  %i, Delay(ns): %i)\n",
    //        testRun->config.numPktsToSend, testRun->config.delayns);
    char configStr[1000];
    configToString(configStr, testRun->config);
    printf("     %s\n", configStr);
    fprintf(fptr, "%s\n", configStr);

    char statsStr[1000];
    statsToString(statsStr, &testRun->stats);
    printf("     %s\n", statsStr);
    fprintf(fptr, "%s\n", statsStr);


    fprintf(fptr, "pkt,timediff\n");
    for(int i=0; i<testRun->numTestData;i++){
        const struct TestData *muh;
        muh = testRun->testData+i;

        if(muh->timeDiff.tv_nsec == 0){
            fprintf(fptr, "%i,%s\n", muh->pkt.seq, "NaN");
        }else {
            if(muh->timeDiff.tv_sec > 0){
                printf("Warning warning FIX me.. diff larger than a second\n");
            }
            fprintf(fptr, "%i,%ld\n", muh->pkt.seq, muh->timeDiff.tv_nsec);
        }
    }
    fclose(fptr);
    printf("    ----Done saving\n");
}