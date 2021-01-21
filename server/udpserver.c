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
#include <timing.h>

#include <logger.h>

#define MAXBUFLEN 1500
#define MAX_LISTEN_SOCKETS 1
#define MAX_THREADS 100000


int             sockfd;


void sendStarOfTest(struct TestRun *run, int sockfd) {

    char newName[MAX_TESTNAME_LEN];
    strncpy(newName, run->config.testName, strlen(run->config.testName));

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


static void*
startDownStreamTests(void* ptr) {
    struct TestRun *run = ptr;
    struct TimingInfo timingInfo;
    struct TestPacket pkt;
    char s[200];
    fiveTupleToString(s, run->fiveTuple);
    LOG_INFO("Starting Downstream Test ---> %s", s);
    logger_flush();

    int currBurstIdx = 0;
    timeStartTest(&timingInfo);

    struct timespec overshoot = {0, 0};
    nap(&run->config.pktConfig.delay, &overshoot);

    sendStarOfTest(run, sockfd);

    uint8_t buf[run->config.pktConfig.pkt_size];

    memset(buf, 67, sizeof(buf));

    int numPkt = 0;
    bool done = false;
    while(!done){
        numPkt++;
        timeSendPacket(&timingInfo);

        uint32_t seq = numPkt;
        fillPacket(&pkt, 0, seq , in_progress_test_cmd,
                   &timingInfo.timeSinceLastPkt, NULL);
        memcpy(buf, &pkt, sizeof(pkt));

        sendPacket(sockfd, (const uint8_t *) buf, sizeof(buf),
                   (const struct sockaddr *) &run->fiveTuple->dst,
                   0, run->config.pktConfig.dscp, 0);

        //Do I sleep or am I bursting..
        sleepBeforeNextBurst(&timingInfo, run->config.pktConfig.pktsInBurst, &currBurstIdx, &run->config.pktConfig.delay);

        timeStartBurst(&timingInfo);

        //Lets prepare fill the next packet buffer with some usefull txData.
        int written = insertResponseData(buf + sizeof(pkt), sizeof(buf) - sizeof(pkt), run);
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
    }
    LOG_INFO("Stopping Downstream Test %s", s);
    return NULL;
}

void
packetHandler(struct ListenConfig* config,
            struct sockaddr*     from_addr,
              __unused void*                cb,
            unsigned char*       buf,
            int                  buflen) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC_RAW, &now);

    struct TestRunManager *mng = config->tInst;

    struct FiveTuple *tuple;
    tuple = makeFiveTuple((const struct sockaddr *)&config->localAddr,
                  from_addr,
                  sockaddr_ipPort(from_addr));

    int res = addTestDataFromBuf(mng, tuple, buf, buflen, &now);

    if(res<0){
        LOG_ERROR("Encountered error while processing incoming UDP packet");
    }
    if(res == 1){
        pthread_t responseThread;
        struct TestRun *run = findTestRun(mng, tuple);
        if( run != NULL) {
            LOG_INFO("Starting new Downstream Test");
            pthread_create(&responseThread, NULL, startDownStreamTests, (void *) run);

        }else{
            LOG_ERROR("Could not find corresponding testrun to start downstream tests");
        }
    }
    free(tuple);
}

void
teardown()
{
    LOG_INFO("Got SIGINT shutting down");
    close(sockfd);
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
pruneAndPrintStatus(void* ptr)
{
    struct TestRunManager *mngr = ptr;
    FILE *fptr = fopen("udpserver_status.txt", "w");

    for (;;) {
        pruneLingeringTestRuns(mngr);
        fptr = freopen(NULL, "w", fptr);
        if(fptr !=NULL) {
        fprintf(fptr, "Running Tests: %i  Mbps : %f  Loss : %i \n",
                getNumberOfActiveTestRuns(mngr),
                 getActiveBwOnAllTestRuns(mngr) / 1000000,
                 getPktLossOnAllTestRuns(mngr));
        fflush(fptr);
        }
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
            {"verbose", 0, 0, 'w'},
            {NULL, 0, NULL, 0}
    };
    if (argc < 1)
    {
        printUsage();
        exit(0);
    }
    int option_index = 0;
    while ( ( c = getopt_long(argc, argv, "hvwi:p:m:",
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
            case 'w':
                logger_initConsoleLogger(stderr);
                LOG_INFO("Logging to stderr");
                break;
            default:
                printf("?? getopt returned character code 0%o ??\n", c);
        }
    }

    char logfile[] = "udpserver.log";

    logger_initFileLogger(logfile, 1024 * 1024, 5);
    logger_autoFlush(100);


    if ( !getLocalInterFaceAddrs( (struct sockaddr*)&listenConfig.localAddr,
                                  listenConfig.interface,
                                  AF_INET,
                                  IPv6_ADDR_NORMAL,
                                  false ) )
    {
        fprintf(stderr, "Error getting IPaddr on %s\n", listenConfig.interface);
        LOG_FATAL("Error getting IPaddr on %s", listenConfig.interface);
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

    LOG_INFO("Up and running on %s", listenConfig.interface);

    pthread_create(&socketListenThread,
                   NULL,
                   socketListenDemux,
                   (void*)&listenConfig);

    pthread_create(&cleanupThread, NULL, pruneAndPrintStatus, (void *) &testRunManager);
    pthread_join(socketListenThread, NULL);
}

