#include <chrono>
#include <thread>
#include <iostream>

#include <stdlib.h>
#include <string.h>
#include <doctest/doctest.h>
#include <udpjitter.h>
#include <fivetuple.h>
#include <timing.h>

#include <doctest/doctest.h>


using namespace std;


uint32_t pktLoss_current_seq = 0;
int64_t pktLoss_current_jitter = 0;

uint32_t pktLoss_curr_start = 0;
uint32_t pktLoss_curr_end = 0;

int pktLoss_current_id=0;

void testrunJitterCB(int id, uint32_t seq, int64_t jitter){
    pktLoss_current_id = id;
    pktLoss_current_seq = seq;
    pktLoss_current_jitter = jitter;
}


void testrunPktLossCB(int id, uint32_t start, uint32_t end){
    pktLoss_curr_start = start;
    pktLoss_curr_end = end;
}




TEST_CASE("packet loss handling") {
    struct sockaddr_storage src, dst;
    sockaddr_initFromString( (struct sockaddr*)&src,
                             "192.168.1.2" );
    sockaddr_initFromString( (struct sockaddr*)&dst,
                             "158.38.48.10" );

    struct TestRunConfig tConf;
    strncpy(tConf.testName, "Doctest\n", MAX_TESTNAME_LEN);
    tConf.pktConfig.tos = 0;
    tConf.pktConfig.pktsInBurst = 0;
    tConf.pktConfig.numPktsToSend = 0;
    tConf.pktConfig.delay = timespec_from_ms(10);
    tConf.pktConfig.pkt_size = 1200;


    struct TestRun run, txRun;
    struct FiveTuple *tuple = makeFiveTuple((struct sockaddr*)&src, (struct sockaddr*)&src, 5004);

    initTestRun(&run, 1, tuple, &tConf, testrunJitterCB, testrunPktLossCB, false);
    initTestRun(&txRun, 2, tuple, &tConf, testrunJitterCB, testrunPktLossCB, false);
    free(tuple);


    uint8_t buf[1200];
    memset(buf, 67, sizeof(buf));

    int written = insertResponseData(buf, sizeof(buf), &run);
    CHECK(written == 0);
    int recvd = extractRespTestData(buf, &txRun);
    CHECK(recvd == 0);

    int seq = 0;
    for(int i=0; i < 10; i++) {
        seq++;
        struct TestPacket tPkt;
        tPkt.seq = seq;
        tPkt.txInterval = 10000;
        tPkt.lastSeqConfirmed = 0;
        run.lastPktTime.tv_sec = 0;
        run.lastPktTime.tv_nsec = 0;

        struct timespec now;
        timespec_from_nsec(&now, 10000+i);
        addTestData(&run, &tPkt, 1200, &now);
        //cout<<"Rx Jitter: "<<current_jitter<<" ("<<current_seq<<")"<<endl;
    }

    written = insertResponseData(buf, sizeof(buf), &run);
    recvd = extractRespTestData(buf, &txRun);
    REQUIRE( pktLoss_current_id == 2);
    REQUIRE(pktLoss_current_jitter == 8);

    //Introduce a lost pkt
    seq++;

    for(int i=0; i < 4; i++) {
        seq++;
        struct TestPacket tPkt;
        tPkt.seq = seq;
        tPkt.txInterval = 10000;
        tPkt.lastSeqConfirmed = 7;
        run.lastPktTime.tv_sec = 0;
        run.lastPktTime.tv_nsec = 0;

        struct timespec now;
        timespec_from_nsec(&now, 10000+i);
        addTestData(&run, &tPkt, 1200, &now);
    }

    REQUIRE( pktLoss_curr_start == 11);
    REQUIRE( pktLoss_curr_end == 11);


    written = insertResponseData(buf, sizeof(buf), &run);



    pktLoss_curr_start = 0;
    pktLoss_curr_end = 0;
    recvd = extractRespTestData(buf, &txRun);

    REQUIRE( pktLoss_curr_start == 11);
    REQUIRE( pktLoss_curr_end == 11);
    pktLoss_curr_start = 0;
    pktLoss_curr_end = 0;

    //Loose 10 pkts
    seq+=10;

    seq++;
    struct TestPacket tPkt;
    tPkt.seq = seq;
    tPkt.txInterval = 10000;
    tPkt.lastSeqConfirmed = seq-1;
    run.lastPktTime.tv_sec = 0;
    run.lastPktTime.tv_nsec = 0;

    struct timespec now;
    timespec_from_nsec(&now, 10000);
    addTestData(&run, &tPkt, 1200, &now);

    REQUIRE( pktLoss_curr_start == 16);
    REQUIRE( pktLoss_curr_end == 25);

    written = insertResponseData(buf, sizeof(buf), &run);
    pktLoss_curr_start = 0;
    pktLoss_curr_end = 0;
    recvd = extractRespTestData(buf, &txRun);
    REQUIRE( pktLoss_curr_start == 0);
    REQUIRE( pktLoss_curr_end == 0);

    freeTestRun(&run);

}

