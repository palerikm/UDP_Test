//
// Created by Pal-Erik Martinsen on 12/11/2020.
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sockethelper.h>

#include <hashmap.h>
#include "packettest.h"

uint64_t TestRun_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    //const struct TestRun *run = item;
    return hashmap_sip(item, sizeof(struct FiveTuple), seed0, seed1);
}

int TestRun_compare(const void *a, const void *b, void *udata) {
    const struct TestRun *ua = a;
    const struct TestRun *ub = b;

    if( ! sockaddr_alike((const struct sockaddr *)&ua->fiveTuple.src,
                   (const struct sockaddr *)&ub->fiveTuple.src)){
        return 1;
    }

    if( ! sockaddr_alike((const struct sockaddr *)&ua->fiveTuple.dst,
                         (const struct sockaddr *)&ub->fiveTuple.dst)){
        return 1;
    }

    if( ua->fiveTuple.port !=  ub->fiveTuple.port){
        return 1;
    }
    return 0;
}


bool TestRun_print_iter(const void *item, void *udata) {
    const struct TestRun *run = item;
    printf("Name: %s", run->config.testName);
    char s[200];
    printf("5tuple: %s\n", fiveTupleToString(s, &run->fiveTuple));
    return true;
}

bool TestRun_complete_iter(const void *item, void *udata) {
    const struct TestRun *run = item;
    if(run->done){
        memcpy(udata, item, sizeof(struct TestRun));
        return false;
    }
    return true;
}

bool TestRun_lingering_iter(const void *item, void *udata) {
    const struct TestRun *testRun = item;
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC_RAW, &now);
    struct timespec elapsed = {0,0};
    timespec_diff(&now, &(*testRun).lastPktTime, &elapsed);
    double sec = (double)elapsed.tv_sec + (double)elapsed.tv_nsec / 1000000000;

    //No data recieved last 5 sec.
    if(sec > 5){
        memcpy(udata, item, sizeof(struct TestRun));
        return false;
    }
    return true;
}

bool TestRun_all_iter(const void *item, void *udata) {
    //const struct TestRun *testRun = item;

    memcpy(udata, item, sizeof(struct TestRun));
    return false;

}

bool TestRun_bw_iter(const void *item, void *udata) {
    const struct TestRun *testRun = item;
    double *mbits = udata;
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC_RAW, &now);
    struct timespec elapsed = {0,0};
    timespec_diff(&now, &(*testRun).stats.startTest, &elapsed);
    double sec = (double)elapsed.tv_sec + (double)elapsed.tv_nsec / 1000000000;
    *mbits += (((*testRun).stats.rcvdBytes * 8) / sec);
    return true;
}
uint32_t fillPacket(struct TestPacket *testPacket, uint32_t srcId, uint32_t seq,
                   uint32_t cmd, const char* testName){
    testPacket->pktCookie = TEST_PKT_COOKIE;
    testPacket->srcId = srcId;
    testPacket->seq = seq;
    testPacket->cmd = cmd;
    if(testName != NULL) {
        strncpy(testPacket->testName, testName, sizeof(testPacket->testName));
    }
    return 0;
}

int initTestRun(struct TestRun *testRun, uint32_t maxNumPkts, struct TestRunConfig *config){

    testRun->maxNumTestData = maxNumPkts;
    testRun->done = false;
    testRun->numTestData = 0;

    memcpy(&testRun->config, config, sizeof(struct TestRunConfig));

    testRun->stats.lostPkts = 0;
    testRun->stats.rcvdPkts = 0;
    testRun->stats.rcvdBytes =0;

    testRun->testData = malloc(sizeof(struct TestData)* testRun->maxNumTestData);
    if(testRun->testData == NULL){
        perror("Error allocating memory for testdata: ");
        return -1;
    }

    return 0;
}

bool pruneLingeringTestRuns(struct TestRunManager *mngr){
    struct TestRun run;
    bool notEarly = hashmap_scan(mngr->map, TestRun_lingering_iter, &run);
    if(!notEarly) {
        freeTestRun(&run);
        hashmap_delete(mngr->map, &run);
    }
    return !notEarly;
}

void pruneAllTestRuns(struct TestRunManager *mngr){
    struct TestRun run;
    bool notEarly = hashmap_scan(mngr->map, TestRun_all_iter, &run);
    while(!notEarly) {
        freeTestRun(&run);
        hashmap_delete(mngr->map, &run);
        notEarly = hashmap_scan(mngr->map, TestRun_all_iter, &run);
    }
}

double getActiveBwOnAllTestRuns(struct TestRunManager *mngr){
    double mbits = 0;
    hashmap_scan(mngr->map, TestRun_bw_iter, &mbits);
    return mbits;
}
int getNumberOfActiveTestRuns(struct TestRunManager *mngr){
    return hashmap_count(mngr->map);
}

void freeTestRunManager(struct TestRunManager *mngr){
  pruneAllTestRuns(mngr);
}
bool saveAndDeleteFinishedTestRuns(struct TestRunManager *mngr, const char *filenameEnding){
    struct TestRun run;
    bool notEarly = hashmap_scan(mngr->map, TestRun_complete_iter, &run);
    if(!notEarly) {
        char filename[100];
        strncpy(filename, run.config.testName, sizeof(filename));
        strncat(filename, filenameEnding, strlen(filenameEnding));
        saveTestDataToFile(&run, filename);
        freeTestRun(&run);
        hashmap_delete(mngr->map, &run);
    }
    return !notEarly;
}

int addTestData(struct TestRun *testRun, const struct TestPacket *testPacket, int pktSize, const struct timespec *now){
    struct timespec timeSinceLastPkt;

    if(testRun == NULL){
        return -1;
    }

    if(testRun->numTestData >= testRun->maxNumTestData){
        testRun->stats.endTest = *now;
        return -2;
    }

    //Start of test?
    if(testPacket->cmd == start_test_cmd){
        testRun->done = false;
        testRun->lastPktTime = *now;
        testRun->stats.startTest = *now;
        return 0;
    }

    if(testRun->numTestData > 0) {
        timespec_diff(now, &testRun->lastPktTime, &timeSinceLastPkt);

        //Did we loose any packets? Or out of order?
        struct TestPacket *lastPkt = &(testRun->testData + testRun->numTestData - 1)->pkt;
        if (testPacket->seq < lastPkt->seq) {
            printf("Todo: Fix out of order handling\n");

        }
        if (testPacket->seq > lastPkt->seq + 1) {
            int lostPkts = (testPacket->seq - lastPkt->seq) - 1;
            printf("Packet loss (%i)\n", lostPkts);
            for (int i = 0; i < lostPkts; i++) {
                struct TestPacket tPkt;
                memset(&tPkt, 0, sizeof(tPkt));
                tPkt.seq = lastPkt->seq + 1 + i;
                struct TestData d;
                d.pkt = tPkt;
                d.timeDiff.tv_sec = 0;
                d.timeDiff.tv_nsec = 0;
                memcpy((testRun->testData + testRun->numTestData), &d, sizeof(struct TestData));
                testRun->numTestData++;
            }
            testRun->stats.lostPkts += lostPkts;
        }
    }else{
        timeSinceLastPkt.tv_sec = 0;
        timeSinceLastPkt.tv_nsec = 0;
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
    testRun->lastPktTime = *now;
    return 0;
}



struct FiveTuple* makeFiveTuple(const struct sockaddr* src,
                                const struct sockaddr* dst,
                                int port){
    struct FiveTuple *fiveTuple;
    fiveTuple = (struct FiveTuple*)malloc(sizeof(struct FiveTuple));
    memset(fiveTuple, 0, sizeof(struct FiveTuple));
    sockaddr_copy((struct sockaddr *)&fiveTuple->src, src);
    sockaddr_copy((struct sockaddr *)&fiveTuple->dst, dst);
    fiveTuple->port = port;
    return fiveTuple;
}

char  *fiveTupleToString(char *str, const struct FiveTuple *tuple){

    char              addrStr[SOCKADDR_MAX_STRLEN];
    sockaddr_toString( (struct sockaddr*)&tuple->src,
                       addrStr,
                       sizeof(addrStr),
                       false );
    strncpy(str, addrStr, sizeof(addrStr));
    strncat(str, " \0", 0);
    sockaddr_toString( (struct sockaddr*)&tuple->dst,
                       addrStr,
                       sizeof(addrStr),
                       false );
    strncat(str, addrStr, strlen(addrStr));
    char result[50];
    sprintf(result, " %i", tuple->port);
    strncat(str, result, strlen(result));
    return str;
}

struct TestRun* findTestRun(struct TestRunManager *mng, const struct FiveTuple *fiveTuple){
    return hashmap_get(mng->map, &(struct TestRun){ .fiveTuple=*fiveTuple });
}

int addTestDataFromBuf(struct TestRunManager *mng, const struct FiveTuple *fiveTuple, const unsigned char* buf, int buflen, const struct timespec *now){
    struct TestPacket *pkt;
    if (buflen<sizeof(struct TestPacket)){
        return 1;
    }

    pkt = (struct TestPacket *)buf;
    if(pkt->pktCookie != TEST_PKT_COOKIE){
        //printf("Not a test pkt! Ignoring\n");
        return 1;
    }
    struct TestRun *run = findTestRun(mng, fiveTuple );

    if(run!=NULL){
        if(pkt->cmd == stop_test_cmd){
            run->stats.endTest = *now;
            run->done = true;
            return 1;
        }
        if(pkt->cmd == start_test_cmd){
            return 1;
        }
        return addTestData(run, pkt, buflen, now);
    }

    if(pkt->cmd == stop_test_cmd){
        return 0;
    }

    if(pkt->cmd == start_test_cmd) {
        struct TestRun *testRun = malloc(sizeof(struct TestRun));
        testRun->fiveTuple = *fiveTuple;
        struct TestRunConfig newConf;
        memcpy(&newConf, &mng->defaultConfig, sizeof(struct TestRunConfig));
        //memcpy(&newConf.fiveTuple, fiveTuple, sizeof(struct FiveTuple));
        strncpy(newConf.testName, pkt->testName, sizeof(pkt->testName));
        initTestRun(testRun, MAX_NUM_RCVD_TEST_PACKETS, &newConf);
        testRun->lastPktTime = *now;
        testRun->stats.startTest = *now;
        hashmap_set(mng->map, testRun);
        return 0;
    }

    return 0;
}

struct TestPacket getNextTestPacket(const struct TestRun *testRun){
    struct TestPacket pkt;
    fillPacket(&pkt, 23, testRun->numTestData, in_progress_test_cmd,
               NULL);
    return pkt;
}

struct TestPacket getEndTestPacket(const struct TestRun *testRun){
    struct TestPacket pkt;
    fillPacket(&pkt, 23, testRun->numTestData, stop_test_cmd,
               (char *)testRun->config.testName);
    return pkt;
}

struct TestPacket getStartTestPacket(const char *testName){
    struct TestPacket pkt;
    fillPacket(&pkt, 23, 0, start_test_cmd,
               testName);
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

    sprintf(result, " Burst: %i", config->pktsInBurst);
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

    char configStr[1000];
    configToString(configStr, &testRun->config);
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
    printf("----Done saving\n");
}