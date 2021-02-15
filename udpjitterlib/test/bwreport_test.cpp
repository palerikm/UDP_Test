
#include <stdlib.h>
#include <string.h>
#include <doctest/doctest.h>
#include <udpjitter.h>
#include <fivetuple.h>
#include <timing.h>
#include <iostream>


void statusCB(double mbps, double ps){
    std::cout<<mbps<<"  "<<ps<<std::endl;
}

TEST_CASE("BW report test") {

    struct TestRunManager *testRunManager = newTestRunManager(statusCB);


    struct sockaddr_storage src, dst;
    sockaddr_initFromString( (struct sockaddr*)&src,
                             "192.168.1.2" );
    sockaddr_initFromString( (struct sockaddr*)&dst,
                             "158.38.48.10" );

    struct TestRunConfig tConf;
    strncpy(tConf.testName, "Doctest\0", MAX_TESTNAME_LEN);
    tConf.pktConfig.tos = 0;
    tConf.pktConfig.pktsInBurst = 0;
    tConf.pktConfig.numPktsToSend = 0;
    tConf.pktConfig.delay = timespec_from_ms(10);
    tConf.pktConfig.pkt_size = 1400;


    struct TestRun tmpRun;
    struct FiveTuple *tuple = makeFiveTuple((struct sockaddr*)&src, (struct sockaddr*)&src, 5004);

    initTestRun(&tmpRun, 1, tuple, &tConf, NULL, NULL, false);
    free(tuple);


    struct TestRun *run = addTestRun(testRunManager, &tmpRun);
    //freeTestRun(&tmpRun);
    run->lastPktTime = {0,0};
    run->stats.startTest = run->lastPktTime;

    struct timespec elapsed = {0,0};
    double mbps = 0;
    double ps = 0;
    for(uint64_t i=1; i < 100000; i++) {
        struct TestPacket tPkt;
        tPkt.seq = i;
        tPkt.txInterval = 10000;
        tPkt.cmd = in_progress_test_cmd;

        run->lastPktTime.tv_sec = 0;
        run->lastPktTime.tv_nsec = 0;

        struct timespec now;
        timespec_from_nsec(&now, 1000000);
        timespec_add(&elapsed, &elapsed, &now);

        addTestData(run, &tPkt, tConf.pktConfig.pkt_size, &elapsed);

        //timespec_sub(&elapsed, &timingInfo.timeBeforeSendPacket, &timingInfo.startOfTest);
        double sec = (double)elapsed.tv_sec + (double)elapsed.tv_nsec / 1000000000;
        mbps = ((double)((tConf.pktConfig.pkt_size *i * 8) / sec)/ 1000000);
        ps = (double)i / sec;

        CHECK(std::to_string(mbps) == std::to_string(11.2));
        CHECK(std::to_string(ps) == std::to_string(1000.0));


        CHECK(std::to_string(TestRunGetBw(run)) == std::to_string(11.2));
        CHECK(std::to_string(TestRunGetPs(run)) == std::to_string(1000.0));
        double allBw = getActiveBwOnAllTestRuns(testRunManager);
        CHECK(std::to_string(allBw) == std::to_string(11.2));

    }

    freeTestRunManager(&testRunManager);

}