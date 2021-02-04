#include <iostream>
#include <chrono>
#include <thread>

#include <stdlib.h>
#include <string.h>
#include <doctest/doctest.h>
#include <udpjitter.h>
#include <fivetuple.h>
#include <timing.h>


using namespace std;


uint32_t current_seq = 0;
int64_t current_jitter = 0;
int current_id=0;

void testrunCB(int id, uint32_t seq, int64_t jitter){
    current_id = id;
    current_seq = seq;
    current_jitter = jitter;
    //if(id == 2){
    //    cout<<"tx Jitter: "<<jitter<<" ("<<seq<<") "<<endl;
   // }
}



TEST_CASE("adding tests to TestRun") {
    struct sockaddr_storage src, dst;
    sockaddr_initFromString( (struct sockaddr*)&src,
                             "192.168.1.2" );
    sockaddr_initFromString( (struct sockaddr*)&dst,
                             "158.38.48.10" );

    struct TestRunConfig tConf;
    strncpy(tConf.testName, "Doctest\n", MAX_TESTNAME_LEN);
    tConf.pktConfig.dscp = 0;
    tConf.pktConfig.pktsInBurst = 0;
    tConf.pktConfig.numPktsToSend = 0;
    tConf.pktConfig.delay = timespec_from_ms(10);
    tConf.pktConfig.pkt_size = 1200;


    struct TestRun run;
    struct FiveTuple *tuple = makeFiveTuple((struct sockaddr*)&src, (struct sockaddr*)&src, 5004);

    initTestRun(&run, 1, tuple, &tConf, testrunCB, false);
    free(tuple);
    //run.TestRun_live_cb = testrunCB;

    for(int i=1; i < 100000; i++) {
        struct TestPacket tPkt;
        tPkt.seq = i;
        tPkt.txInterval = 10000;

        run.lastPktTime.tv_sec = 0;
        run.lastPktTime.tv_nsec = 0;

        struct timespec now;
        timespec_from_nsec(&now, 10000+i);
        addTestData(&run, &tPkt, 1200, &now);

        CHECK(run.numTestData == i);
        CHECK(run.stats.rcvdPkts == i);
        CHECK(run.stats.lostPkts == 0);
        CHECK(run.stats.rcvdBytes == i*tConf.pktConfig.pkt_size);
        if(i>1) {
            CHECK(current_jitter == i);
        }
    }
    freeTestRun(&run);
}

TEST_CASE("downstream tests (Insert Response data)"){
    struct sockaddr_storage src, dst;
    sockaddr_initFromString( (struct sockaddr*)&src,
                             "192.168.1.2" );
    sockaddr_initFromString( (struct sockaddr*)&dst,
                             "158.38.48.10" );

    struct TestRunConfig tConf;
    strncpy(tConf.testName, "Doctest\n", MAX_TESTNAME_LEN);
    tConf.pktConfig.dscp = 0;
    tConf.pktConfig.pktsInBurst = 0;
    tConf.pktConfig.numPktsToSend = 0;
    tConf.pktConfig.delay = timespec_from_ms(10);
    tConf.pktConfig.pkt_size = 1200;


    struct TestRun run, txRun;
    struct FiveTuple *tuple = makeFiveTuple((struct sockaddr*)&src, (struct sockaddr*)&src, 5004);

    initTestRun(&run, 1, tuple, &tConf, testrunCB, false);
    initTestRun(&txRun, 2, tuple, &tConf, testrunCB, false);
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
    CHECK(run.numTestData == 10);
    recvd = extractRespTestData(buf, &txRun);
    CHECK(recvd == written);
    CHECK( txRun.numTestData == 10);
    CHECK( current_id == 2);
    CHECK(current_jitter == 9);


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
    written = insertResponseData(buf, sizeof(buf), &run);
    CHECK(run.numTestData == 7);
    for(int i=0; i < 4; i++) {
        seq++;
        struct TestPacket tPkt;
        tPkt.seq = seq;
        tPkt.txInterval = 10000;
        tPkt.lastSeqConfirmed = seq-1;
        run.lastPktTime.tv_sec = 0;
        run.lastPktTime.tv_nsec = 0;

        struct timespec now;
        timespec_from_nsec(&now, 10000+i);
        addTestData(&run, &tPkt, 1200, &now);
    }
    written = insertResponseData(buf, sizeof(buf), &run);
    CHECK(run.numTestData == 1);

    freeTestRun(&run);

}
