#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>


#include <pthread.h>
#include <getopt.h>

#include <stdbool.h>
#include <packettest.h>

#include "hashmap.h"
#include "../include/iphelper.h"
#include "../include/sockethelper.h"
#include "../include/udptestcommon.h"

#define MAXBUFLEN 1500
#define MAX_LISTEN_SOCKETS 1
#define MAX_THREADS 100000


int             sockfd;


void sendStarOfTest(struct TestRun *run, int sockfd) {

    char newName[MAX_TESTNAME_LEN];
    strncpy(newName, run->config.testName, strlen(run->config.testName));
   // strncat(newName, "_resp\0", 7);

    struct TestPacket startPkt = getStartTestPacket(newName);

    struct timespec now, remaining;
    uint8_t endBuf[run->config.pktConfig.pkt_size];
    memcpy(endBuf, &startPkt, sizeof(struct TestPacket));
    memcpy(endBuf+sizeof(startPkt), &run->config.pktConfig, sizeof(struct TestRunPktConfig));
    for(int j=0;j<10;j++){
        clock_gettime(CLOCK_MONOTONIC_RAW, &now);
        sendPacket(sockfd, (const uint8_t *)&endBuf, sizeof(endBuf),
                   (const struct sockaddr*)&run->fiveTuple->dst,
                   0, run->config.pktConfig.dscp, 0 );

        nanosleep(&run->config.pktConfig.delay, &remaining);
    }
}
void sendEndOfTest(struct TestRun *run, int num, int sockfd) {
    //Send End of Test a few times...

    struct TestPacket startPkt = getEndTestPacket(run->config.testName, num);

    struct timespec now, remaining;
    uint8_t endBuf[run->config.pktConfig.pkt_size];
    memcpy(endBuf, &startPkt, sizeof(struct TestPacket));
    memcpy(endBuf+sizeof(startPkt), &run->config.pktConfig, sizeof(struct TestRunPktConfig));
    for(int j=0;j<10;j++){
        clock_gettime(CLOCK_MONOTONIC_RAW, &now);
        sendPacket(sockfd, (const uint8_t *)&endBuf, sizeof(endBuf),
                   (const struct sockaddr*)&run->fiveTuple->dst,
                   0, run->config.pktConfig.dscp, 0 );

        nanosleep(&run->config.pktConfig.delay, &remaining);
    }
}

static int insertResponseData(uint8_t *buf, size_t bufsize, int seq, const struct TestRun *run ) {
    memset(buf+sizeof(struct TestPacket), 0, bufsize - sizeof(struct TestPacket));
   // printf("Num Testdata: %i, last idx confirmed: %i\n", run->numTestData, run->resp.lastIdxConfirmed);
    int numRespItemsInQueue = run->numTestData - run->resp.lastIdxConfirmed;
    int numRespItemsThatFitInBuffer = bufsize/sizeof(struct TestRunPktResponse);

    int currentWritePos = sizeof(int32_t);
    int32_t written = -1;
    bool done = false;
    while(!done){
        written++;
    //for(written = 0; written<numRespItemsInQueue;written++){
        if(written>=numRespItemsThatFitInBuffer || written>=numRespItemsInQueue){
            done = true;
        }
        struct TestRunPktResponse respPkt;
        struct TestData *tData = &run->testData[run->resp.lastIdxConfirmed+written];
        respPkt.pktCookie = TEST_RESP_PKT_COOKIE;
        respPkt.seq = tData->pkt.seq;
     //   printf("seq: %i\n", respPkt.seq);
        respPkt.jitter_ns = tData->jitter_ns;
        respPkt.txDiff = tData->pkt.txDiff;
        respPkt.rxDiff = tData->rxDiff;
        memcpy(buf+currentWritePos+sizeof(respPkt)*written, &respPkt, sizeof(respPkt));

    }
    memcpy(buf, &written, sizeof(written));
  //   printf("Written: %i (%i) In queue: %i\n", written, numRespItemsThatFitInBuffer, numRespItemsInQueue);
    //int offset = seq - run->testData[run->resp.lastIdxConfirmed + written -1].pkt.seq;
   // printf("Offset: %i\n", offset);
    return  written;
}

static void*
startDownStreamTests(void* ptr){
  struct TestRun *run = ptr;

    struct timespec startBurst,endBurst, inBurst, timeBeforeSendPacket, timeLastPacket;
    struct TestPacket pkt;

    int currBurstIdx = 0;
    clock_gettime(CLOCK_MONOTONIC_RAW, &startBurst);

    struct timespec overshoot = {0,0};
    nap(&run->config.pktConfig.delay, &overshoot);

    sendStarOfTest(run, sockfd);

    clock_gettime(CLOCK_MONOTONIC_RAW, &timeLastPacket);
    int numPkt = 0;
    uint8_t buf[run->config.pktConfig.pkt_size];
    //uint8_t buf[1400];

    memset(buf, 67, sizeof(buf));
    int numPkts_to_send = run->config.pktConfig.numPktsToSend;

    for(numPkt=0 ;numPkt<numPkts_to_send;numPkt++){
        clock_gettime(CLOCK_MONOTONIC_RAW, &timeBeforeSendPacket);
        struct timespec timeSinceLastPkt;
        timespec_sub(&timeSinceLastPkt, &timeBeforeSendPacket, &timeLastPacket);
        //Get next test packet to send.
        //pkt = getNextTestPacket(run, &timeBeforeSendPacket);
        fillPacket(&pkt, 0, numPkt+1, in_progress_test_cmd,
                   &timeSinceLastPkt,NULL,NULL);
        memcpy(buf, &pkt, sizeof(pkt));

        sendPacket(sockfd, (const uint8_t *) buf, sizeof(buf),
                   (const struct sockaddr *) &run->fiveTuple->dst,
                   0, run->config.pktConfig.dscp, 0);

        //Do I sleep or am I bursting..
        if(currBurstIdx < run->config.pktConfig.pktsInBurst){
            currBurstIdx++;
        }else {
            currBurstIdx = 0;
            struct timespec delay = {0,0};
            clock_gettime(CLOCK_MONOTONIC_RAW, &endBurst);
            timespec_sub(&inBurst, &endBurst, &startBurst);
            delay.tv_sec = run->config.pktConfig.delay.tv_sec - inBurst.tv_sec - overshoot.tv_sec;
            delay.tv_nsec = run->config.pktConfig.delay.tv_nsec - inBurst.tv_nsec - overshoot.tv_nsec;
            nap(&delay, &overshoot);
            //Staring a new burst when we start at top of for loop.
            clock_gettime(CLOCK_MONOTONIC_RAW, &startBurst);
        }
        timeLastPacket = timeBeforeSendPacket;

        //Lets prepare fill the next packet buffer with some usefull data.
        insertResponseData(buf+sizeof(pkt), sizeof(buf)-sizeof(pkt), numPkt+1, run);

    }//End of main test run. Just some cleanup remaining.
    sendEndOfTest(run, numPkt, sockfd);
    return NULL;
}

void
packetHandler(struct ListenConfig* config,
            struct sockaddr*     from_addr,
            void*                cb,
            unsigned char*       buf,
            int                  buflen) {
    struct timespec now, result;
    clock_gettime(CLOCK_MONOTONIC_RAW, &now);

    struct TestRunManager *mng = config->tInst;

    struct FiveTuple *tuple;
    tuple = makeFiveTuple((const struct sockaddr *)&config->localAddr,
                  from_addr,
                  sockaddr_ipPort(from_addr));


    int res = addTestDataFromBuf(mng, tuple, buf, buflen, &now);

    if(res<0){
        printf("Encountered error while processing incoming UDP packet\n");
    }
    if(res == 1){
        pthread_t responseThread;
        struct TestRun *run = findTestRun(mng, tuple);
        if( run != NULL) {
            printf("Starting downstream testrun: %s\n", run->config.testName);
            pthread_create(&responseThread, NULL, startDownStreamTests, (void *) run);
        }else{
            printf("Could not find corresponding testrun to start downstream tests\n");
        }

    }

    free(tuple);
}

void
teardown()
{
    close(sockfd);
    printf("Quitting..\n");
    exit(0);
}

void
printUsage()
{
    printf("Usage: udpserver [options] host\n");
    printf("Options: \n");
    printf("  -i, --interface               Interface\n");
    printf("  -p, --port                    Listen port\n");
    printf("  -v, --version                 Prints version number\n");
    printf("  -h, --help                    Print help text\n");
    exit(0);
}
_Noreturn static void*
saveAndMoveOn(void* ptr)
{
    struct TestRunManager *mngr = ptr;
    char filenameEnding[] = "_server_results.txt";
    for (;; )
    {
        //saveAndDeleteFinishedTestRuns(mngr, filenameEnding);
        pruneLingeringTestRuns(mngr);

        printf("\r Running Tests: %i ", getNumberOfActiveTestRuns(mngr));
        printf(" Mbps : %f ", getActiveBwOnAllTestRuns(mngr)/1000000);
        printf(" Loss : %i ", getPktLossOnAllTestRuns(mngr));
        printf(" Max Jitter %f ", (double)getMaxJitterTestRuns(mngr)/NSEC_PER_SEC);
        usleep(10000);
    }
}

int
main(int   argc,
     char* argv[])
{
    struct TestRunConfig testConfig;
    memset(&testConfig, 0, sizeof(testConfig));

    pthread_t socketListenThread;
    pthread_t cleanupThread;

    struct ListenConfig listenConfig;

    signal(SIGINT, teardown);
    int c;
    /* int                 digit_optind = 0; */
    /* set config to default values */
    strncpy(listenConfig.interface, "default", 7);
    listenConfig.port          = 3478;


    static struct option long_options[] = {
            {"interface", 1, 0, 'i'},
            {"port", 1, 0, 'p'},
            {"help", 0, 0, 'h'},
            {"version", 0, 0, 'v'},
            {NULL, 0, NULL, 0}
    };
    if (argc < 1)
    {
        printUsage();
        exit(0);
    }
    int option_index = 0;
    while ( ( c = getopt_long(argc, argv, "hvi:p:m:",
                              long_options, &option_index) ) != -1 )
    {
        /* int this_option_optind = optind ? optind : 1; */
        switch (c)
        {
            case 'i':
                strncpy(listenConfig.interface, optarg, max_iface_len);
                break;
            case 'p':
                listenConfig.port = atoi(optarg);
                break;
            case 'h':
                printUsage();
                break;
            case 'v':
                printf("Version %s\n", "Nothing to see here");
                exit(0);
                break;
            default:
                printf("?? getopt returned character code 0%o ??\n", c);
        }
    }

    if ( !getLocalInterFaceAddrs( (struct sockaddr*)&listenConfig.localAddr,
                                  listenConfig.interface,
                                  AF_INET,
                                  IPv6_ADDR_NORMAL,
                                  false ) )
    {
        printf("Error getting IPaddr on %s\n", listenConfig.interface);
        exit(1);
    }

    sockfd = createLocalSocket(AF_INET,
                               (struct sockaddr*)&listenConfig.localAddr,
                               SOCK_DGRAM,
                               listenConfig.port);

    listenConfig.socketConfig[0].sockfd = sockfd;
    listenConfig.pkt_handler          = packetHandler;
    listenConfig.numSockets             = 1;

    struct TestRunManager testRunManager;
    //testRunManager.defaultConfig = testConfig;
    testRunManager.map = hashmap_new(sizeof(struct TestRun), 0, 0, 0,
                                     TestRun_hash, TestRun_compare, NULL);


    listenConfig.tInst = &testRunManager;

    pthread_create(&socketListenThread,
                   NULL,
                   socketListenDemux,
                   (void*)&listenConfig);

    pthread_create(&cleanupThread, NULL, saveAndMoveOn, (void*)&testRunManager);
    pause();

}

