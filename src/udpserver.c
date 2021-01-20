#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>


#include <pthread.h>
#include <getopt.h>

#include <stdbool.h>

#include <udpjitter.h>
#include <iphelper.h>
#include <sockethelper.h>
#include <testrun.h>

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

    struct TestPacket startPkt = getEndTestPacket(num);

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




static void*
startDownStreamTests(void* ptr) {
    struct TestRun *run = ptr;

    //struct timespec startBurst, endBurst, inBurst, timeBeforeSendPacket, timeLastPacket;
    struct TimingInfo timingInfo;
    struct TestPacket pkt;


    int currBurstIdx = 0;
    timeStartTest(&timingInfo);
    //clock_gettime(CLOCK_MONOTONIC_RAW, &startBurst);

    struct timespec overshoot = {0, 0};
    nap(&run->config.pktConfig.delay, &overshoot);

    sendStarOfTest(run, sockfd);

    //clock_gettime(CLOCK_MONOTONIC_RAW, &timeLastPacket);

    uint8_t buf[run->config.pktConfig.pkt_size];


    memset(buf, 67, sizeof(buf));
   // int numPkts_to_send = run->config.pktConfig.numPktsToSend;
    int numPkt = 0;
    bool done = false;
    while(!done){
        numPkt++;
        timeSendPacket(&timingInfo);
        //clock_gettime(CLOCK_MONOTONIC_RAW, &timeBeforeSendPacket);
        //struct timespec timeSinceLastPkt;
        //timespec_sub(&timeSinceLastPkt, &timeBeforeSendPacket, &timeLastPacket);


        //uint32_t seq = numPkt>=numPkts_to_send ? numPkts_to_send : numPkt +1;
        uint32_t seq = numPkt;
        fillPacket(&pkt, 0, seq , in_progress_test_cmd,
                   &timingInfo.timeSinceLastPkt, NULL);
        memcpy(buf, &pkt, sizeof(pkt));

        sendPacket(sockfd, (const uint8_t *) buf, sizeof(buf),
                   (const struct sockaddr *) &run->fiveTuple->dst,
                   0, run->config.pktConfig.dscp, 0);

        //Do I sleep or am I bursting..
        sleepBeforeNextBurst(&timingInfo, run->config.pktConfig.pktsInBurst, &currBurstIdx, &run->config.pktConfig.delay);
        //struct timespec delay = getBurstDelay(&run->config, &startBurst, &endBurst, &inBurst, &currBurstIdx, &overshoot);
        //nap(&delay, &overshoot);
        //Staring a new burst when we start at top of for loop.
        timeStartBurst(&timingInfo);
        //clock_gettime(CLOCK_MONOTONIC_RAW, &startBurst);
        //timeLastPacket = timeBeforeSendPacket;

        //Lets prepare fill the next packet buffer with some usefull txData.
        int written = insertResponseData(buf + sizeof(pkt), sizeof(buf) - sizeof(pkt), seq, run);
        if(written > 0) {
            pthread_mutex_lock(&run->lock);
            //So how much can we move back?
            //Find the packet with the last conf seq
            int idx = run->numTestData - written;
            for(int i = 0; i < written; i++){
                if( run->testData[idx+i].pkt.seq == run->resp.lastSeqConfirmed){
                    memcpy(run->testData, &run->testData[idx+i], sizeof(struct TestData)*i);
                    run->numTestData = i;
                    break;
                }
            }
            pthread_mutex_unlock(&run->lock);
        }

        //Bail out if we have not recieved a packet the last second.
        //Todo: Make this a function of the delay in the config?
        struct timespec elapsed = {0,0};
        timespec_sub(&elapsed, &timingInfo.startBurst, &run->lastPktTime);
        if (elapsed.tv_sec > 1){
            done = true;
        }
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
    listenConfig.port          = 5004;


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

    initTestRunManager(&testRunManager);

    listenConfig.tInst = &testRunManager;

    pthread_create(&socketListenThread,
                   NULL,
                   socketListenDemux,
                   (void*)&listenConfig);

    pthread_create(&cleanupThread, NULL, saveAndMoveOn, (void*)&testRunManager);
    pause();

}

