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

static int insertResponseData(uint8_t *buf, size_t bufsize, const struct TestRun *run ) {
    int32_t sendSize = run->numTestData - run->resp.lastIdxConfirmed;
    // printf("Num TestData: %i, Last confirmed: %i\n", run->numTestData, run->resp.lastIdxConfirmed);
    int32_t testPacketSize = sizeof(struct TestPacket);
    int32_t dataPos = testPacketSize + sizeof(int32_t);
    int32_t max_num_to_send = (bufsize - dataPos)/sizeof(struct TestData);
    int32_t sendActual = sendSize<max_num_to_send ? sendSize : max_num_to_send;
    if(sendActual > 0){
        memcpy(buf + testPacketSize, &sendActual, sizeof(int32_t));
        memcpy(buf+dataPos,
               run->testData + run->resp.lastIdxConfirmed,
               sizeof(struct TestData)*sendActual);
    }else{
        memset(buf+sizeof(struct TestPacket), 0, bufsize - sizeof(struct TestPacket));
    }

    return sendActual;
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

    for(numPkt=0 ;numPkt<run->config.pktConfig.numPktsToSend;numPkt++){
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
        insertResponseData(buf, sizeof(buf), run);

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
        pthread_create(&responseThread, NULL, startDownStreamTests, (void *) run);
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

