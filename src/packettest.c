//
// Created by Pal-Erik Martinsen on 12/11/2020.
//

#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <sockethelper.h>

#include <hashmap.h>
#include <pthread.h>
#include "packettest.h"
#include "udptestcommon.h"

uint64_t TestRun_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    const struct TestRun *run = item;
    return hashmap_sip(run->fiveTuple, sizeof(struct FiveTuple), seed0, seed1);
}

int TestRun_compare(const void *a, const void *b, void *udata) {
    const struct TestRun *ua = a;
    const struct TestRun *ub = b;

    if( ! sockaddr_alike((const struct sockaddr *)&ua->fiveTuple->src,
                   (const struct sockaddr *)&ub->fiveTuple->src)){
        return 1;
    }

    if( ! sockaddr_alike((const struct sockaddr *)&ua->fiveTuple->dst,
                         (const struct sockaddr *)&ub->fiveTuple->dst)){
        return 1;
    }

    if( ua->fiveTuple->port !=  ub->fiveTuple->port){
        return 1;
    }
    return 0;
}


bool TestRun_print_iter(const void *item, void *udata) {
    const struct TestRun *run = item;
    printf("Name: %s ", run->config.testName);
    char s[200];
    printf("5tuple: %s  ", fiveTupleToString(s, run->fiveTuple));
    return true;
}

bool TestRun_complete_iter(const void *item, void *udata) {
    const struct TestRun *run = item;
    if(run->done){
        memcpy(udata, &item, sizeof(void *));
        return false;
    }
    return true;
}

bool TestRun_lingering_iter(const void *item, void *udata) {
    const struct TestRun *testRun = item;

    //if(testRun->done){
    //    return true;
   // }
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC_RAW, &now);
    struct timespec elapsed = {0,0};
    timespec_sub(&elapsed, &now, &(*testRun).lastPktTime);
    double sec = (double)elapsed.tv_sec + (double)elapsed.tv_nsec / 1000000000;

    //No data recieved last 5 sec.
   // printf(" %f, ", sec);
    if(sec > 5){
        memcpy(udata, &item, sizeof(void *));
        return false;
    }
    return true;
}

bool TestRun_all_iter(const void *item, void *udata) {
    memcpy(udata, &item, sizeof(void *));
    return false;
}

bool TestRun_bw_iter(const void *item, void *udata) {
    const struct TestRun *testRun = item;
    double *mbits = udata;
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC_RAW, &now);
    struct timespec elapsed = {0,0};
    timespec_sub(&elapsed, &now, &(*testRun).stats.startTest);
    double sec = (double)elapsed.tv_sec + (double)elapsed.tv_nsec / 1000000000;
    *mbits += (((*testRun).stats.rcvdBytes * 8) / sec);
    return true;
}

bool TestRun_loss_iter(const void *item, void *udata) {
    const struct TestRun *testRun = item;
    uint32_t *loss = udata;
    *loss += testRun->stats.lostPkts;
    return true;
}

bool TestRun_max_jitter_iter(const void *item, void *udata) {
    const struct TestRun *testRun = item;
    int64_t *max_jitter = udata;
    if( llabs(*max_jitter) < llabs(testRun->stats.jitterInfo.maxJitter)) {
        *max_jitter = testRun->stats.jitterInfo.maxJitter;
    }
    return true;
}

uint32_t fillPacket(struct TestPacket *testPacket, uint32_t srcId, uint32_t seq,
                   uint32_t cmd, struct timespec *tDiff, struct TestRunResponse *resp){
    testPacket->pktCookie = TEST_PKT_COOKIE;
    testPacket->srcId = srcId;
    testPacket->seq = seq;
    testPacket->cmd = cmd;
    if(tDiff != NULL){
        testPacket->txDiff = *tDiff;
    }
    if(resp != NULL) {
        testPacket->resp = *resp;
    }else{
        testPacket->resp.lastSeqConfirmed = -1;
    }

    return 0;
}

int initTestRun(struct TestRun *testRun, uint32_t maxNumPkts,
                const struct FiveTuple *fiveTuple,
                struct TestRunConfig *config, bool liveUpdate){
    printf("\n---- Init Testrun: %s  ------\n", config->testName);
    if(liveUpdate){
        char filename[100];
        char fileEnding[] = "_live.csv\0";
        strncpy(filename, config->testName, sizeof(filename));
        strncat(filename, fileEnding, strlen(fileEnding));
        printf("Trying to open: %s", filename);
        testRun->fptr = fopen(filename,"w");
        if(testRun->fptr == NULL)
        {
            printf("Error!");
            exit(1);
        }
        saveCsvHeader(testRun->fptr);
    }else{
        testRun->fptr = NULL;
    }

    testRun->fiveTuple = makeFiveTuple((struct sockaddr*)&fiveTuple->src, (struct sockaddr*)&fiveTuple->dst, fiveTuple->port);
    testRun->maxNumTestData = maxNumPkts;
    testRun->done = false;
    testRun->numTestData = 0;

    memcpy(&testRun->config, config, sizeof(struct TestRunConfig));
//    sockaddr_copy((struct sockaddr*)&testRun->config.remoteAddr, (struct sockaddr*)&fiveTuple->dst);
    testRun->stats.lostPkts = 0;
    testRun->stats.rcvdPkts = 0;
    testRun->stats.rcvdBytes =0;

    testRun->testData = malloc(sizeof(struct TestData)* testRun->maxNumTestData);
    if(testRun->testData == NULL){
        perror("Error allocating memory for testdata: ");
        return -1;
    }

    testRun->resp.lastSeqConfirmed = 0;

    if (pthread_mutex_init(&testRun->lock, NULL) != 0) {
        printf("\n TestRun mutex init has failed\n");
        return 1;
    }
    return 0;
}

bool pruneLingeringTestRuns(struct TestRunManager *mngr){
    struct TestRun *run;
    bool notEarly = hashmap_scan(mngr->map, TestRun_lingering_iter, &run);
    if(!notEarly) {
        freeTestRun(hashmap_delete(mngr->map, run));
    }
    return !notEarly;
}

void pruneAllTestRuns(struct TestRunManager *mngr){
    struct TestRun *run;
    bool notEarly = hashmap_scan(mngr->map, TestRun_all_iter, &run);
    while(!notEarly) {
        freeTestRun(hashmap_delete(mngr->map, run));
        notEarly = hashmap_scan(mngr->map, TestRun_all_iter, &run);
    }
}
uint32_t getPktLossOnAllTestRuns(struct TestRunManager *mngr){
    //struct TestRun *run;
    uint32_t loss = 0;
    bool notEarly = hashmap_scan(mngr->map, TestRun_loss_iter, &loss);
    return loss;
}

double getActiveBwOnAllTestRuns(struct TestRunManager *mngr){
    double mbits = 0;
    hashmap_scan(mngr->map, TestRun_bw_iter, &mbits);
    return mbits;
}
int64_t getMaxJitterTestRuns(struct TestRunManager *mngr){
    int64_t maxJitter = 0;
    hashmap_scan(mngr->map, TestRun_max_jitter_iter, &maxJitter);
    return maxJitter;
}


int getNumberOfActiveTestRuns(struct TestRunManager *mngr){
    return hashmap_count(mngr->map);
}

void freeTestRunManager(struct TestRunManager *mngr){
  pruneAllTestRuns(mngr);
}
bool saveAndDeleteFinishedTestRuns(struct TestRunManager *mngr, const char *filenameEnding){
    struct TestRun *run;
    bool notEarly = hashmap_scan(mngr->map, TestRun_complete_iter, &run);
    if(!notEarly) {
        char filename[100];
        strncpy(filename, run->config.testName, sizeof(filename));
        strncat(filename, filenameEnding, strlen(filenameEnding));
        saveTestRunToFile(run, filename);
        freeTestRun(hashmap_delete(mngr->map, run));
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
        timespec_sub(&timeSinceLastPkt, now, &testRun->lastPktTime);
        testRun->resp = testPacket->resp;
        //Did we loose any packets? Or out of order?
        struct TestPacket *lastPkt = &testRun->testData[testRun->numTestData-1].pkt;
        if (testPacket->seq < lastPkt->seq) {
            //printf("Todo: Fix out of order handling\n");
            return -3;

        }
        if (testPacket->seq == lastPkt->seq) {
            //printf("Todo: Duplicate packet\n");
            return -4;

        }

        if (testPacket->seq > lastPkt->seq + 1) {
            int lostPkts = (testPacket->seq - lastPkt->seq) - 1;
            printf("Lost packets: %i (%i,%i)\n", lostPkts, testPacket->seq, lastPkt->seq);
            for (int i = 0; i < lostPkts; i++) {
                struct TestPacket tPkt;
                memset(&tPkt, 0, sizeof(tPkt));
                tPkt.seq = lastPkt->seq + 1 + i;
                struct TestData d;
                d.pkt = tPkt;
                d.rxDiff.tv_sec = 0;
                d.rxDiff.tv_nsec = 0;
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


    // Calculate jitter based on time since last received packet and the
    // sending variation happening on the client(txDiff)
    struct timespec jitter = {0,0};
    timespec_sub(&jitter, &timeSinceLastPkt, &testPacket->txDiff);
    d.jitter_ns = timespec_to_nsec(&jitter);

    d.rxDiff = timeSinceLastPkt;


    memcpy((testRun->testData+testRun->numTestData), &d, sizeof(struct TestData));
    testRun->numTestData++;
    testRun->lastPktTime = *now;

    if(testRun->fptr != NULL){
        saveTestData(&d, testRun->fptr);
        fflush(testRun->fptr);
    }
    if(llabs(d.jitter_ns) > llabs(testRun->stats.jitterInfo.maxJitter)){
        testRun->stats.jitterInfo.maxJitter = d.jitter_ns;
    }

    int64_t a = 0;
    for(int i=0;i < testRun->numTestData;i++){
        a += ((testRun->testData)+1)->jitter_ns;
    }
    testRun->stats.jitterInfo.avgJitter = a/testRun->numTestData;

    return 0;
}

int extractRespTestData(const unsigned char *buf, struct TestRun *run) {
    int32_t numTestData = 0;
    uint32_t currPosition = 0;
    memcpy(&numTestData, buf, sizeof(int32_t));
    if(numTestData > 0){
        currPosition+= sizeof(int32_t);

        for(int i=0;i<numTestData;i++){
            struct TestRunPktResponse respPkt;
            memcpy(&respPkt, buf + currPosition, sizeof(respPkt));
            if(respPkt.pktCookie != TEST_RESP_PKT_COOKIE) {
                //printf("Not a valid RSP COOKIE\n");
                break;
            }


            struct TestPacket tPkt;
            tPkt.seq = respPkt.seq;
            struct timespec txts = {0, respPkt.txDiff};
            tPkt.txDiff = txts;

           // printf("Seq %i\n", respPkt.seq);
            run->lastPktTime.tv_sec = 0;
            run->lastPktTime.tv_nsec = 0;

            struct timespec rxts = {0, respPkt.rxDiff};
            int res = addTestData(run, &tPkt, sizeof(tPkt), &rxts);


            currPosition+=sizeof(respPkt);
        }
    }
    return numTestData;

}

int insertResponseData(uint8_t *buf, size_t bufsize, int seq, struct TestRun *run ) {
    memset(buf+sizeof(struct TestPacket), 0, bufsize - sizeof(struct TestPacket));
    if(run->numTestData < 1){
        return 0;
    }
    int lastSeq = run->testData[run->numTestData-1].pkt.seq;
    int numRespItemsInQueue =  lastSeq - run->resp.lastSeqConfirmed;
    int numRespItemsThatFitInBuffer = bufsize/sizeof(struct TestRunPktResponse);
       int currentWritePos = sizeof(int32_t);
    int32_t written = -1;
    bool done = false;
    int idx = 0;
    while(!done){
        written++;
        struct TestRunPktResponse respPkt;
        idx = run->numTestData-numRespItemsInQueue+written;
        struct TestData *tData = &run->testData[idx];

        respPkt.pktCookie = TEST_RESP_PKT_COOKIE;
        respPkt.seq = tData->pkt.seq;
        respPkt.jitter_ns = tData->jitter_ns;
        respPkt.txDiff = timespec_to_nsec(&tData->pkt.txDiff);
        respPkt.rxDiff = timespec_to_nsec(&tData->rxDiff);

        memcpy(buf+currentWritePos+sizeof(respPkt)*written, &respPkt, sizeof(respPkt));

        if(written>=numRespItemsThatFitInBuffer || written>=numRespItemsInQueue){
            done = true;
        }

    }
    memcpy(buf, &written, sizeof(written));

    return  written;
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
    strncat(str, " \0", 1);
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

struct TestRun* findTestRun(struct TestRunManager *mng, struct FiveTuple *fiveTuple){
    return hashmap_get(mng->map, &(struct TestRun){ .fiveTuple=fiveTuple });
}

int addTestDataFromBuf(struct TestRunManager *mng,
                       struct FiveTuple *fiveTuple,
                       const unsigned char* buf,
                       int buflen,
                       const struct timespec *now){

    struct TestPacket *pkt;
    if (buflen<sizeof(struct TestPacket)){
        return -1;
    }

    pkt = (struct TestPacket *)buf;
    if(pkt->pktCookie != TEST_PKT_COOKIE){
        //printf("Not a test pkt! Ignoring\n");
        return -2;
    }
    struct TestRun *run = findTestRun(mng, fiveTuple );

    if(run!=NULL){
        if(pkt->cmd == stop_test_cmd){
            run->stats.endTest = *now;
            run->done = true;
            return 0;
        }
        if(pkt->cmd == start_test_cmd){
            return 0;
        }
        if(pkt->cmd == transport_resp_cmd){
            return 2;
        }
        pthread_mutex_lock(&run->lock);
        addTestData(run, pkt, buflen, now);
        pthread_mutex_unlock(&run->lock);
        return sizeof(*pkt);
    }

    if(pkt->cmd == stop_test_cmd){
        return 0;
    }

    if(pkt->cmd == transport_resp_cmd){
        return 2;
    }

    if(pkt->cmd == start_test_cmd) {
        struct TestRun testRun;

        struct TestRunConfig newConf;

        struct TestRunPktConfig pktConfig;
        memcpy(&pktConfig, buf+sizeof(struct TestPacket), sizeof(struct TestRunPktConfig));
        newConf.pktConfig = pktConfig;

        initTestRun(&testRun, pktConfig.numPktsToSend, fiveTuple, &newConf, false);

        testRun.lastPktTime = *now;
        testRun.stats.startTest = *now;
        hashmap_set(mng->map, &testRun);
        return 1;
    }

    return 0;
}

struct TestPacket getEndTestPacket(int num){
    struct TestPacket pkt;
    fillPacket(&pkt, 23, num, stop_test_cmd,
               NULL, NULL);
    return pkt;
}

struct TestPacket getStartTestPacket(){
    struct TestPacket pkt;
    fillPacket(&pkt, 23, 0, start_test_cmd,
               NULL, NULL);
    return pkt;
}

int freeTestRun(struct TestRun *testRun){

    if(testRun != NULL){
        pthread_mutex_destroy(&testRun->lock);
        if(testRun->testData != NULL) {
            free(testRun->testData);
        }
        if(testRun->fiveTuple != NULL) {
            free(testRun->fiveTuple);
        }
    }
    return 0;
}

int configToString(char* configStr, const struct TestRunConfig *config){

//    char              addrStr[SOCKADDR_MAX_STRLEN];
//    sockaddr_toString( (struct sockaddr*)&config->localAddr,
//                               addrStr,
//                               sizeof(addrStr),
//                               false );
//    strncpy(configStr, addrStr, sizeof(addrStr));
//    strncat(configStr, " (\0", 4);
//    strncat(configStr, config->interface, strlen(config->interface));
//    strncat(configStr, ") -> \0", 6);
//
//    sockaddr_toString( (struct sockaddr*)&config->remoteAddr,
//                       addrStr,
//                       sizeof(addrStr),
//                       true );
//    strncat(configStr, addrStr, strlen(addrStr));
    char result[100];
    memset(result, 0, sizeof(result));
    sprintf(result, " PktSize: %i", config->pktConfig.pkt_size);
    strncpy(configStr, result, strlen(result)+1);

    sprintf(result, " DSCP: %#x", config->pktConfig.dscp);
    strncat(configStr, result, strlen(result));

    sprintf(result, " Delay: %ld", config->pktConfig.delay.tv_nsec/1000000L);
    strncat(configStr, result, strlen(result));

    sprintf(result, " Burst: %i", config->pktConfig.pktsInBurst);
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
    timespec_sub(&elapsed, &stats->endTest, &stats->startTest);

    double sec = (double)elapsed.tv_sec + (double)elapsed.tv_nsec / NSEC_PER_SEC;

    sprintf(result, " Time : %f sec", sec);
    strncat(configStr, result, strlen(result));

    sprintf(result, " (Mbps : %f, p/s: %f) ", (((stats->rcvdBytes*8)/sec)/1000000), stats->rcvdPkts/sec);
    strncat(configStr, result, strlen(result));

    return 0;

}

void saveTestData(const struct TestData *tData, FILE *fptr){
    int64_t rxDiff = timespec_to_nsec(&tData->rxDiff);
    int64_t txDiff = timespec_to_nsec(&tData->pkt.txDiff);
    fprintf(fptr, "%i,", tData->pkt.seq);
    if(rxDiff == 0){
        fprintf(fptr, "%s,","NaN");
    }else{
        fprintf(fptr, "%" PRId64 ",", rxDiff);
    }
    if(txDiff == 0){
        fprintf(fptr, "%s,","NaN");
    }else{
        fprintf(fptr, "%" PRId64 ",", txDiff);
    }

    fprintf(fptr, "%" PRId64, tData->jitter_ns);
    fprintf(fptr, "\n");
}
void saveCsvHeader(FILE *fptr){
    fprintf(fptr, "seq,txDiff,rxDiff,jitter\n");
}

void saveTestRunConfigToFile(const struct TestRun *testRun, FILE *fptr) {
    char fiveTplStr[200];
    fiveTupleToString(fiveTplStr,testRun->fiveTuple );
    printf("    %s  ", fiveTplStr);
    fprintf(fptr,"    %s  ", fiveTplStr);
    char configStr[1000];
    configToString(configStr, &testRun->config);
    printf("     %s\n", configStr);
    fprintf(fptr, "%s\n", configStr);
}

void saveTestRunStatsToFile(const struct TestRunStatistics *testRunStats, FILE *fptr) {
    char statsStr[1000];
    statsToString(statsStr, testRunStats);
    printf("     %s\n", statsStr);
    fprintf(fptr, "%s\n", statsStr);
}

void saveTestRunToFile(const struct TestRun *testRun, const char* filename) {
    printf("Saving--- %s (%i)\n", filename, testRun->numTestData);
    FILE *fptr;
    fptr = fopen(filename,"w");
    if(fptr == NULL)
    {
        printf("Error!");
        exit(1);
    }

    saveTestRunConfigToFile(testRun, fptr);
    saveTestRunStatsToFile(&testRun->stats, fptr);
    saveCsvHeader(fptr);
    for(int i=0; i<testRun->numTestData;i++){
        const struct TestData *muh;
        muh = testRun->testData+i;
        saveTestData(muh, fptr);

    }
    fclose(fptr);
    printf("----Done saving\n");
}