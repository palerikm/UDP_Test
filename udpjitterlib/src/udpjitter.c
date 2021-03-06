#include <iso646.h>
//
// Created by Pal-Erik Martinsen on 12/11/2020.
//

#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <sockaddrutil.h>
#include <hashmap.h>
#include <pthread.h>
#include "udpjitter.h"
#include "timing.h"
#include "fivetuple.h"


void saveTestRunToFile(const struct TestRun *testRun, const char* filename);
void saveTestData(const struct TestData *tData, FILE *fptr);

uint64_t TestRun_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    const struct TestRun *run = item;
    return hashmap_sip(run->fiveTuple, sizeof(struct FiveTuple), seed0, seed1);
}

int TestRun_compare(const void *a, const void *b, void *udata) {
    const struct TestRun *ua = a;
    const struct TestRun *ub = b;

    if ( !fiveTupleAlike(ua->fiveTuple, ub->fiveTuple)) {
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

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC_RAW, &now);
    struct timespec elapsed = {0,0};
    timespec_sub(&elapsed, &now, &(*testRun).lastPktTime);
    double sec = (double)elapsed.tv_sec + (double)elapsed.tv_nsec / 1000000000;

    //No txData recieved last 5 sec.
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


double TestRunGetBw(const struct TestRun *run){
    struct timespec elapsed = {0,0};
    timespec_sub(&elapsed, &run->lastPktTime, &run->stats.startTest);
    double sec = (double)elapsed.tv_sec + ((double)elapsed.tv_nsec / 1000000000);
    return (((double)(run->stats.rcvdBytes * 8) / sec))/ 1000000;
}

double TestRunGetPs(const struct TestRun *run){
    struct timespec elapsed = {0,0};
    timespec_sub(&elapsed, &run->lastPktTime, &run->stats.startTest);
    double sec = (double)elapsed.tv_sec + ((double)elapsed.tv_nsec / 1000000000);
    return ((double)(run->stats.rcvdPkts) / sec);
}

bool TestRun_bw_iter(const void *item, void *udata) {
    const struct TestRun *testRun = item;
    double *mbits = udata;
    *mbits += TestRunGetBw(testRun);
    return true;
}

bool TestRun_loss_iter(const void *item, void *udata) {
    const struct TestRun *testRun = item;
    uint32_t *loss = udata;
    *loss += testRun->stats.lostPkts;
    return true;
}


uint32_t fillPacket(struct TestPacket *testPacket, uint32_t seq,
                   uint32_t cmd, int64_t tDiff, int32_t lastSeqConfirmed){
    testPacket->pktCookie = TEST_PKT_COOKIE;
    testPacket->seq = seq;
    testPacket->cmd = cmd;

    testPacket->txInterval = tDiff;

    testPacket->lastSeqConfirmed = lastSeqConfirmed;

    return 0;
}

struct TestRun* addTestRun(struct TestRunManager *mng, struct TestRun *tRun) {
    if( hashmap_set(mng->map, tRun) == NULL){
        if( hashmap_oom(mng->map) ){
            printf("System out of memory\n");
        }
    }
    return findTestRun(mng, tRun->fiveTuple);
}


void initTestRunManager(struct TestRunManager *testRunManager, void (*statusCb)(double, double)) {
    (*testRunManager).map = hashmap_new(sizeof(struct TestRun), 0, 0, 0,
                                        TestRun_hash, TestRun_compare, NULL);
    (*testRunManager).done = false;
    (*testRunManager).TestRun_status_cb = statusCb;
    //(*testRunManager).TestRunLiveJitterCb = NULL;

}

struct TestRunManager* newTestRunManager(void (*statusCb)(double, double)) {
    struct TestRunManager *mng = malloc(sizeof(struct TestRunManager));

    initTestRunManager(mng, statusCb);
    return mng;

}
void saveCsvHeader(FILE *fptr){
    fprintf(fptr, "seq,txInterval,rxInterval,jitter\n");
}

int initTestRun(struct TestRun *testRun,int32_t id,
                const struct FiveTuple *fiveTuple,
                struct TestRunConfig *config,
                void (*jitterCb)(int, uint32_t, int64_t),
                void (*pktLossCb)(int, uint32_t, uint32_t),
                bool liveUpdate){
    testRun->TestRunLiveJitterCb = jitterCb;
    testRun->TestRunLivePktLossCb = pktLossCb;
    if(liveUpdate){
        char filename[100];
        char fileEnding[] = "_live.csv\0";
        strncpy(filename, config->testName, sizeof(filename));
        strncat(filename, fileEnding, strlen(fileEnding));
        printf("Saving results in: %s\n", filename);
        testRun->fptr = fopen(filename,"w");
        if(testRun->fptr == NULL)
        {
            printf("Error!");
            exit(5);
        }
        saveCsvHeader(testRun->fptr);
    }else{
        testRun->fptr = NULL;
    }

    testRun->fiveTuple = makeFiveTuple((struct sockaddr*)&fiveTuple->src, (struct sockaddr*)&fiveTuple->dst, fiveTuple->port);
    testRun->id = id;
    if(config->pktConfig.numPktsToSend == 0) {
        testRun->maxNumTestData = TEST_DATA_INCREMENT_SIZE;
    }else{
        testRun->maxNumTestData = config->pktConfig.numPktsToSend;
    }
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

    testRun->lastSeqConfirmed = 0;

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
    uint32_t loss = 0;
    hashmap_scan(mngr->map, TestRun_loss_iter, &loss);
    return loss;
}

double getActiveBwOnAllTestRuns(struct TestRunManager *mngr){
    double mbits = 0;
    hashmap_scan(mngr->map, TestRun_bw_iter, &mbits);
    return mbits;
}



int getNumberOfActiveTestRuns(struct TestRunManager *mngr){
    return hashmap_count(mngr->map);
}

void freeTestRunManager(struct TestRunManager **mngr) {
    if(*mngr!=NULL) {
        pruneAllTestRuns(*mngr);
        free(*mngr);
        *mngr = NULL;
    }
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
    int64_t rxLocalInterval;

    if(testRun == NULL){
       // printf("  TestRun is NULL");
        return -1;
    }
    if(testPacket == NULL){
        return -1;
    }

    if(testRun->numTestData >= testRun->maxNumTestData){
        testRun->maxNumTestData+= TEST_DATA_INCREMENT_SIZE;
        testRun->testData = realloc(testRun->testData, sizeof(struct TestData)* testRun->maxNumTestData );
        if(testRun->testData == NULL ) {
            testRun->stats.endTest = *now;
            printf("  To much testData %i out of %i Bailing..\n",testRun->numTestData, testRun->maxNumTestData);
            return -2;
        }
    }

    //Start of test?
    if(testPacket->cmd == start_test_cmd){
        testRun->done = false;
        testRun->lastPktTime = *now;
        testRun->stats.startTest = *now;
        return 0;
    }

    if(testRun->numTestData > 0) {
        struct timespec ts;
        timespec_sub(&ts, now, &testRun->lastPktTime);
        rxLocalInterval = timespec_to_nsec(&ts);

        testRun->lastSeqConfirmed = testPacket->lastSeqConfirmed;
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
            if(testRun->numTestData + lostPkts >= testRun->maxNumTestData){
                testRun->maxNumTestData+= TEST_DATA_INCREMENT_SIZE;
                if( lostPkts > TEST_DATA_INCREMENT_SIZE){
                    printf("Ouch, number of lost packets increasing faster than I can handle (%i)", lostPkts);
                    exit(1);
                }
                testRun->testData = realloc(testRun->testData, sizeof(struct TestData)* testRun->maxNumTestData );
                if(testRun->testData == NULL ) {
                    testRun->stats.endTest = *now;
                    printf("  To much testData %i out of %i Bailing..\n",testRun->numTestData, testRun->maxNumTestData);
                    return -2;
                }
            }

            for (int i = 0; i < lostPkts; i++) {
                struct TestPacket tPkt;
                memset(&tPkt, 0, sizeof(tPkt));
                tPkt.seq = lastPkt->seq + 1 + i;
                tPkt.txInterval = -1;
                struct TestData d;
                d.pkt = tPkt;
                memcpy((testRun->testData + testRun->numTestData), &d, sizeof(struct TestData));
                testRun->numTestData++;
            }
            testRun->stats.lostPkts += lostPkts;
            if(testRun->TestRunLivePktLossCb != NULL){
                testRun->TestRunLivePktLossCb(testRun->id, lastPkt->seq+1, testPacket->seq-1);
            }
        }
    }else{
        rxLocalInterval = 0;
    }

    testRun->stats.rcvdPkts++;
    testRun->stats.rcvdBytes+=pktSize;

    struct TestData d;
    d.pkt = *testPacket;

    // Calculate jitter based on time since last received packet and the
    // sending variation happening on the client(txInterval)
    d.jitter_ns = rxLocalInterval - testPacket->txInterval;

    memcpy((testRun->testData+testRun->numTestData), &d, sizeof(struct TestData));
    testRun->numTestData++;
    testRun->lastPktTime = *now;

    if(testRun->fptr != NULL){
        saveTestData(&d, testRun->fptr);
        fflush(testRun->fptr);
    }
    if(testRun->TestRunLiveJitterCb != NULL){
        testRun->TestRunLiveJitterCb(testRun->id, d.pkt.seq, d.jitter_ns);
    }
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
               // currPosition+=sizeof(respPkt);
                break;
            }
            if(respPkt.txInterval_ns < 0){
                currPosition+=sizeof(respPkt);
               continue;
            }
            struct TestPacket tPkt;
            tPkt.seq = respPkt.seq;
            tPkt.txInterval = respPkt.txInterval_ns;
            tPkt.cmd = in_progress_test_cmd;

            //To reuse the addTestDataFunction for response data
            //we set lastPktTime in the testrun to 0 and set "now"
            //to transmitInterval + the jitter value.
            struct timespec last = {0,0};
            run->lastPktTime = last;

           // printf("(TX) RespPkt jitter: %lli (%lli)\n", respPkt.jitter_ms, respPkt.txInterval_ns);
            struct timespec now;
            timespec_from_nsec(&now, respPkt.jitter_ns + respPkt.txInterval_ns);
            addTestData(run, &tPkt, sizeof(tPkt), &now);
            currPosition+=sizeof(respPkt);
        }
    }
    return numTestData;

}

int insertResponseData(uint8_t *buf, size_t bufsize, struct TestRun *run ) {

    memset(buf+sizeof(struct TestPacket), 0, bufsize - sizeof(struct TestPacket));
    if(run->numTestData < 1 ){
        int zero = 0;
        memcpy(buf, &zero, sizeof(int32_t));
        return 0;
    }

    int numRespItemsThatFitInBuffer = bufsize/sizeof(struct TestRunPktResponse);
    //We do not want to grow into eternity...
    if( numRespItemsThatFitInBuffer < run->numTestData ){
        if( numRespItemsThatFitInBuffer > 0){
            int idx = numRespItemsThatFitInBuffer/2;
            memmove(run->testData, &run->testData[idx], sizeof(struct TestData)*(run->numTestData-idx));
            run->numTestData -= idx;
        }else{
            //Heh no responses possible.. Sender should increase buffer size...
        }
    }

    int lastSeq = run->testData[run->numTestData-1].pkt.seq;
    int numRespItemsInQueue =  lastSeq - run->lastSeqConfirmed;

    int wantToWrite = numRespItemsInQueue < numRespItemsThatFitInBuffer ? numRespItemsInQueue : numRespItemsThatFitInBuffer;

    int toWrite = wantToWrite < run->numTestData ? wantToWrite : run->numTestData;
    int currentWritePos = sizeof(int32_t);

    for(int i=0; i<toWrite-1;i++){
        struct TestRunPktResponse respPkt;
        struct TestData *tData = &run->testData[run->numTestData-toWrite+i];
        respPkt.pktCookie = TEST_RESP_PKT_COOKIE;
        respPkt.seq = tData->pkt.seq;
        respPkt.jitter_ns = tData->jitter_ns;
        respPkt.txInterval_ns = tData->pkt.txInterval;
        memcpy(buf+currentWritePos+sizeof(respPkt)*i, &respPkt, sizeof(respPkt));
    }

    memcpy(buf, &toWrite, sizeof(toWrite));

    if(toWrite > 0) {
        pthread_mutex_lock(&run->lock);
        //So how much can we move back?
        //Find the packet with the last conf seq
        int lastConfSeqIdx = 0;
        for(int i = 0; i < run->numTestData; i++){
            int seq = run->testData[i].pkt.seq;
            if (seq == run->lastSeqConfirmed){
                lastConfSeqIdx = i;
                break;
            }
        }

        if (lastConfSeqIdx > 0){
            memmove(run->testData, &run->testData[lastConfSeqIdx+1], sizeof(struct TestData)*(run->numTestData-lastConfSeqIdx+1));
            run->numTestData -= lastConfSeqIdx+1;
        }
        pthread_mutex_unlock(&run->lock);
    }
    return  toWrite;
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

        initTestRun(&testRun, 0, fiveTuple, &newConf, NULL, NULL, false);

        testRun.lastPktTime = *now;
        testRun.stats.startTest = *now;
        hashmap_set(mng->map, &testRun);
        return 1;
    }

    return 0;
}

struct TestPacket getEndTestPacket(int num){
    struct TestPacket pkt;
    fillPacket(&pkt, num, stop_test_cmd,
               0, 0);
    return pkt;
}

struct TestPacket getStartTestPacket(){
    struct TestPacket pkt;
    fillPacket(&pkt, 0, start_test_cmd,
               0, 0);
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
    char result[100];
    memset(result, 0, sizeof(result));
    sprintf(result, " PktSize: %i", config->pktConfig.pkt_size);
    strncpy(configStr, result, strlen(result)+1);

    sprintf(result, " DSCP: %#x", config->pktConfig.tos);
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
    sprintf(result, "Pkts: %llu", stats->rcvdPkts);
    strncat(configStr, result, strlen(result));

    sprintf(result, " Lost pkts: %i", stats->lostPkts);
    strncat(configStr, result, strlen(result));

    sprintf(result, " Bytes rcvd: %llu", stats->rcvdBytes);
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
    int64_t txDiff = tData->pkt.txInterval;
    fprintf(fptr, "%i,", tData->pkt.seq);

    if(txDiff == 0){
        fprintf(fptr, "%s,","NaN");
    }else{
        fprintf(fptr, "%" PRId64 ",", txDiff);
    }

    fprintf(fptr, "%" PRId64, tData->jitter_ns);
    fprintf(fptr, "\n");
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
        exit(5);
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