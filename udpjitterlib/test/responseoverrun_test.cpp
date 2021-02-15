
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
uint32_t respover_current_seq = 0;
int64_t respover_current_jitter = 0;

uint32_t respover_curr_start = 0;
uint32_t respover_curr_end = 0;

int respover_current_id=0;

void respoverJitterCB(int id, uint32_t seq, int64_t jitter){
    respover_current_id = id;
    respover_current_seq = seq;
    respover_current_jitter = jitter;
}


void respoverPktLossCB(int id, uint32_t start, uint32_t end){
    printf("Pkt loss %i-%i\n", start, end);
    respover_curr_start = start;
    respover_curr_end = end;
}





TEST_CASE("response overrun") {
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

    initTestRun(&run, 1, tuple, &tConf, respoverJitterCB, respoverPktLossCB, false);
    initTestRun(&txRun, 2, tuple, &tConf, respoverJitterCB, respoverPktLossCB, false);
    free(tuple);


    uint8_t buf[1200];
    memset(buf, 67, sizeof(buf));

    int seq = 0;
    for(int i=0; i < 70; i++) {
        seq++;
        struct TestPacket tPkt;
        tPkt.seq = seq;
        tPkt.txInterval = 10000;
        tPkt.lastSeqConfirmed = 0;
        tPkt.cmd = in_progress_test_cmd;
        run.lastPktTime.tv_sec = 0;
        run.lastPktTime.tv_nsec = 0;

        struct timespec now;
        timespec_from_nsec(&now, 10000+i);
        addTestData(&run, &tPkt, 1200, &now);
        //cout<<"Rx Jitter: "<<current_jitter<<" ("<<current_seq<<")"<<endl;
    }

    int written = insertResponseData(buf, sizeof(buf), &run);
    CHECK( written == 45);
    CHECK( run.numTestData == 45 );
    int recvd = extractRespTestData(buf, &txRun);
    CHECK( txRun.numTestData == 44 );

    CHECK(recvd == 45);

    for(int i=0; i < 20; i++) {
        seq++;
        struct TestPacket tPkt;
        tPkt.seq = seq;
        tPkt.txInterval = 10000;
        tPkt.lastSeqConfirmed = 50;
        tPkt.cmd = in_progress_test_cmd;
        run.lastPktTime.tv_sec = 0;
        run.lastPktTime.tv_nsec = 0;

        struct timespec now;
        timespec_from_nsec(&now, 10000+i);
        addTestData(&run, &tPkt, 1200, &now);
        //cout<<"Rx Jitter: "<<current_jitter<<" ("<<current_seq<<")"<<endl;
    }

    written = insertResponseData(buf, sizeof(buf), &run);
    CHECK( written == 40);

}