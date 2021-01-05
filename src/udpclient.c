#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
#include <getopt.h>
#include <string.h>
#include <pthread.h>
#include <packettest.h>

#include "iphelper.h"
#include "sockethelper.h"
#include "udptestcommon.h"


static struct ListenConfig listenConfig;


struct TestRunResponse getResponse(const struct TestRun *respRun) {
    struct TestRunResponse r;
    if(respRun->numTestData > 1) {
        r.lastSeqConfirmed = respRun->testData[respRun->numTestData - 1].pkt.seq;
    }else{
        r.lastSeqConfirmed = 0;
    }
    return r;
}

struct TestRun *
addRxTestRun(const struct TestRunConfig *testRunConfig, struct TestRunManager *testRunManager){
    struct FiveTuple *rxFiveTuple;
    rxFiveTuple = makeFiveTuple((struct sockaddr *)&listenConfig.remoteAddr,
                                (struct sockaddr *)&listenConfig.localAddr, listenConfig.port);

    struct TestRun *rxTestRun = malloc(sizeof(struct TestRun));
    struct TestRunConfig rxConfig;
    memcpy(&rxConfig, testRunConfig, sizeof(rxConfig));
    strncat(rxConfig.testName, "_rx\0", 5);
    initTestRun(rxTestRun, rxConfig.pktConfig.numPktsToSend, rxFiveTuple, &rxConfig, true);
    clock_gettime(CLOCK_MONOTONIC_RAW, &rxTestRun->lastPktTime);
    rxTestRun->stats.startTest = rxTestRun->lastPktTime;

    hashmap_set((*testRunManager).map, rxTestRun);
    free(rxFiveTuple);
    return rxTestRun;
}

struct TestRun *
addTxTestRun(const struct TestRunConfig *testRunConfig, struct TestRunManager *testRunManager) {
    struct FiveTuple *txFiveTuple;
    txFiveTuple = makeFiveTuple((struct sockaddr *)&listenConfig.localAddr,
                                   (struct sockaddr *)&listenConfig.remoteAddr, listenConfig.port);
    struct TestRun *txTestRun = malloc(sizeof(struct TestRun));
    struct TestRunConfig txConfig;
    memcpy(&txConfig, testRunConfig, sizeof(txConfig));
    strncat(txConfig.testName, "_tx\0", 5);
    initTestRun(txTestRun, txConfig.pktConfig.numPktsToSend, txFiveTuple, &txConfig, true);
    clock_gettime(CLOCK_MONOTONIC_RAW, &txTestRun->lastPktTime);
    txTestRun->stats.startTest = txTestRun->lastPktTime;
    hashmap_set((*testRunManager).map, txTestRun);
    free(txFiveTuple);

    return txTestRun;
}

void
packetHandler(struct ListenConfig* config,
              struct sockaddr*     from_addr,
              void*                cb,
              unsigned char*       buf,
              int                  buflen) {
    struct timespec now, result;
    clock_gettime(CLOCK_MONOTONIC_RAW, &now);
    struct TestRunManager *mngr = config->tInst;
    struct FiveTuple *tuple;
    tuple = makeFiveTuple((const struct sockaddr *)&config->remoteAddr,
                          (const struct sockaddr *)&config->localAddr,
                          config->port);


    int pos = addTestDataFromBuf(mngr, tuple, buf, buflen, &now);
   // printf("Got a packet..(%i)\n", pos);

    if(pos<0){
        printf("Encountered error while processing incoming UDP packet\n");
    }
    //lets check if there is more stuff in the packet..
    if(pos > 5) {
        fflush(stdout);
        struct FiveTuple *txtuple = makeFiveTuple((const struct sockaddr *) &config->localAddr,
                                                  (const struct sockaddr *) &config->remoteAddr,
                                                  config->port);
        struct TestRun *run = findTestRun(mngr, txtuple);
        if(run != NULL) {
            extractRespTestData(buf + pos, run);
        }

        free(txtuple);
    }

    free(tuple);

}


void sendEndOfTest(struct TestRunConfig *cfg, struct FiveTuple *fiveTuple, int num, int sockfd) {

    struct TestPacket pkt = getEndTestPacket(num);


    struct timespec now, remaining;
    uint8_t endBuf[cfg->pktConfig.pkt_size];
    memcpy(endBuf, &pkt, sizeof(struct TestPacket));

    for(int j=0;j<10;j++){
        clock_gettime(CLOCK_MONOTONIC_RAW, &now);
        sendPacket(sockfd, (const uint8_t *)&endBuf, sizeof(endBuf),
                   (const struct sockaddr*)&fiveTuple->dst,
                   0, cfg->pktConfig.dscp, 0 );

        nanosleep(&cfg->pktConfig.delay, &remaining);
    }
}

void sendStarOfTest(struct TestRunConfig *cfg, struct FiveTuple *fiveTuple, int sockfd) {
    //Send End of Test a few times...

    struct TestPacket startPkt = getStartTestPacket();

    struct timespec now, remaining;
    uint8_t endBuf[cfg->pktConfig.pkt_size];
    memcpy(endBuf, &startPkt, sizeof(struct TestPacket));
    memcpy(endBuf+sizeof(startPkt), &cfg->pktConfig, sizeof(struct TestRunPktConfig));
    for(int j=0;j<10;j++){
        clock_gettime(CLOCK_MONOTONIC_RAW, &now);
        sendPacket(sockfd, (const uint8_t *)&endBuf, sizeof(endBuf),
                   (const struct sockaddr*)&fiveTuple->dst,
                   0, cfg->pktConfig.dscp, 0 );


       //addTestDataFromBuf(mng, fiveTuple,
       //                    endBuf, sizeof(endBuf), &now);

        nanosleep(&cfg->pktConfig.delay, &remaining);
    }
}

static void
teardown()
{
  for (int i = 0; i < listenConfig.numSockets; i++)
  {
    close(listenConfig.socketConfig[i].sockfd);
  }

  exit(0);
}

void
printUsage(char* prgName)
{
  printf("Usage: %s [options] <ip/FQDN>\n", prgName);
  printf("Options: \n");
  printf("  -i, --interface           Interface\n");
  printf("  -p <port>, --port <port>  Destination port\n");
  printf("  -d <ms>, --delay <ms>     Delay after each sendto\n");
  printf("  -b <num>, --burst <num>   Packets to send in each burst\n");
  printf("  -t <0xNN> --dscp <0xNN>   DSCP/Diffserv value\n");
  printf("  -s <bytes> --size <bytes> Size of packet payload in bytes\n");
  printf("  -v, --version             Print version number\n");
  printf("  -h, --help                Print help text\n");
  exit(0);
}

void printStats(struct TestRunManager *mngr, const struct timespec *now, const struct timespec *startOfTest, int numPkts, int numPktsToSend, int pktSize ) {
    if(numPkts % 100 == 0){
        if( getNumberOfActiveTestRuns(mngr)< 2){
            printf("Hmmm, not receiving from the server. Do not feed the black holes on the Internet!\n");
            printf("Did you send to the right server? (Or check your FW settings)\n");
            exit(1);
        }
        struct timespec elapsed = {0,0};
        timespec_sub(&elapsed, now, startOfTest);
        double sec = (double)elapsed.tv_sec + (double)elapsed.tv_nsec / 1000000000;
        int done = ((double)numPkts / (double)numPktsToSend)*100;
        printf("\r(Mbps : %f, p/s: %f Progress: %i %%)", (((pktSize *numPkts * 8) / sec) / 1000000), numPkts / sec, done);
        fflush(stdout);
    }
}

void configure(struct TestRunConfig* config,
          struct ListenConfig* listenConfig,
          int                   argc,
          char*                 argv[])
{
    int c;
    /* int                 digit_optind = 0; */
    /* set config to default values */
    strncpy(listenConfig->interface, "default", 7);
    listenConfig->port  = 5004;
    config->pktConfig.numPktsToSend = 3000;
    config->pktConfig.delay.tv_sec = 0;
    config->pktConfig.delay.tv_nsec = 20000000L;
    config->pktConfig.pktsInBurst = 1;
    config->pktConfig.dscp = 0;
    config->pktConfig.pkt_size = 1200;


    int iseed = (unsigned int)time(NULL);
    srand(iseed);

    static const int a='a', z='z';
    for(int i=0; i<MAX_TESTNAME_LEN; i++) {
        config->testName[i] = rand() % (z - a + 1) + a;
    }
    config->testName[MAX_TESTNAME_LEN-1] = '\0';

    static struct option long_options[] = {
        {"interface", 1, 0, 'i'},
        {"name", 1, 0, 'w'},
        {"port", 1, 0, 'p'},
        {"pkts", 1, 0, 'n'},
        {"delay", 1, 0, 'd'},
        {"burst", 1, 0, 'b'},
        {"dscp", 1, 0, 't'},
        {"size", 1, 0, 's'},
        {"help", 0, 0, 'h'},
        {"version", 0, 0, 'v'},
        {NULL, 0, NULL, 0}
    };
    if (argc < 2){
        printUsage(argv[0]);
        exit(0);
    }
    int option_index = 0;
    while ( ( c = getopt_long(argc, argv, "hvi:w:p:o:n:d:b:t:s:",
                            long_options, &option_index) ) != -1 )
    {
    /* int this_option_optind = optind ? optind : 1; */
    switch (c)
    {
        case 'i':
          strncpy(listenConfig->interface, optarg, max_iface_len);
          break;
        case 'w':
            memset(config->testName, 0, MAX_TESTNAME_LEN);
            strncpy(config->testName, optarg, MAX_TESTNAME_LEN-1);
            break;
        case 'p':
          listenConfig->port = atoi(optarg);
          break;
        case 'd':
            config->pktConfig.delay.tv_nsec = atoi(optarg)*1000000L;
          break;
        case 'b':
            config->pktConfig.pktsInBurst = atoi(optarg);
            break;
        case 't':
            config->pktConfig.dscp = strtoul(optarg, NULL, 16);
        case 's':
            config->pktConfig.pkt_size = atoi(optarg);
            break;
        case 'n':
            config->pktConfig.numPktsToSend = atoi(optarg);
            break;
        case 'h':
          printUsage(argv[0]);
          break;
        case 'v':
          printf("Version \n");
          exit(0);
        default:
          printf("?? getopt returned character code 0%o ??\n", c);
          exit(0);
        }
    }
    if (optind < argc){
        if ( !getRemoteIpAddr( (struct sockaddr*)&listenConfig->remoteAddr,
                               argv[optind++],
                               listenConfig->port ) ){
          printf("Error getting remote IPaddr");
          exit(1);
        }
    }


  if ( !getLocalInterFaceAddrs( (struct sockaddr*)&listenConfig->localAddr,
                                listenConfig->interface,
                               listenConfig->remoteAddr.ss_family,
                                IPv6_ADDR_NORMAL,
                                false ) )
  {
    printf("Error getting IPaddr on %s\n", listenConfig->interface);
    exit(1);
  }

}


int
setupSocket(struct ListenConfig *lconf, const struct ListenConfig* sconfig)
{


    int sockfd = createLocalSocket(sconfig->remoteAddr.ss_family,
                                   (struct sockaddr*)&sconfig->localAddr,
                                   SOCK_DGRAM,
                                   0);

    lconf->socketConfig[0].sockfd = sockfd;
    lconf->numSockets++;

  return lconf->numSockets;
}


int startListenThread(struct TestRunManager *testRunManager, const struct ListenConfig* lconfig) {
    pthread_t socketListenThread;
    listenConfig.pkt_handler          = packetHandler;
    listenConfig.tInst = testRunManager;
    pthread_create(&socketListenThread,
                   NULL,
                   socketListenDemux,
                   (void*)lconfig);

    return listenConfig.socketConfig[0].sockfd;

}

int runTests(int sockfd, struct FiveTuple *txFiveTuple,
         struct TestRunConfig *testRunConfig, struct TestRunManager *testRunManager) {
    int numPkt = -1;
    int numPkts_to_send = testRunConfig->pktConfig.numPktsToSend;
    uint8_t buf[(*testRunConfig).pktConfig.pkt_size];
    memset(&buf, 43, sizeof(buf));
    bool done = false;
    struct TestPacket pkt;
    int currBurstIdx = 0;



    struct TestRun *respRun = findTestRun(testRunManager, txFiveTuple);
    if(respRun == NULL){
        printf("\nSomething terrible has happened! Cant find the response testrun!\n");
        char ft[200];
        printf("FiveTuple: %s\n", fiveTupleToString(ft, txFiveTuple));
        exit(1);
    }


    struct TimingInfo timingInfo;
    memset(&timingInfo, 0, sizeof(timingInfo));
    clock_gettime(CLOCK_MONOTONIC_RAW, &timingInfo.startBurst);
    timeStartTest(&timingInfo);
    timeStartBurst(&timingInfo);
    while(!done){
        numPkt++;
        struct TestRunResponse r = getResponse(respRun);

        timeSendPacket(&timingInfo);

        uint32_t seq = numPkt >= numPkts_to_send ? numPkts_to_send : numPkt + 1;
        fillPacket(&pkt, 23, seq, in_progress_test_cmd,
                   &timingInfo.timeSinceLastPkt, &r);
        memcpy(buf, &pkt, sizeof(pkt));

        sendPacket(sockfd, (const uint8_t *) &buf, sizeof(buf),
                   (const struct sockaddr *) &txFiveTuple->dst,
                   0, (*testRunConfig).pktConfig.dscp, 0);

        printStats(testRunManager, &timingInfo.timeBeforeSendPacket, &timingInfo.startOfTest, numPkt, (*testRunConfig).pktConfig.numPktsToSend, (*testRunConfig).pktConfig.pkt_size );
        //Do I sleep or am I bursting..
        sleepBeforeNextBurst(&timingInfo, (*testRunConfig).pktConfig.pktsInBurst, &currBurstIdx, &(*testRunConfig).pktConfig.delay);

        //Staring a new burst when we start at top of for loop.
        timeStartBurst(&timingInfo);

        if(numPkt > numPkts_to_send){

            if(r.lastSeqConfirmed == (*testRunConfig).pktConfig.numPktsToSend) {
                struct TestRun *txrun = findTestRun(testRunManager, txFiveTuple);
                txrun->done = true;
                done = true;
            }
        }

    }//End of main test run. Just some cleanup remaining.
    return numPkt;
}

void waitForResponses(struct TestRunConfig *testRunConfig, struct TestRunManager *testRunManager) {
    bool done = false;
    char fileEnding[] = "_testrun.txt\0";

    while(!done){
         int num = getNumberOfActiveTestRuns(testRunManager);
         if(  num > 0 ){
             saveAndDeleteFinishedTestRuns(testRunManager, fileEnding);
             struct timespec overshoot = {0,0};
             nap(&(*testRunConfig).pktConfig.delay, &overshoot);
         }else{
             done = true;
         }
     }
}

void addTxAndRxTests(struct TestRunConfig *testRunConfig, struct TestRunManager *testRunManager) {
    struct TestRun *txTestRun = addTxTestRun(testRunConfig, testRunManager);
    struct TestRun *rxTestRun = addRxTestRun(testRunConfig, testRunManager);
    free(rxTestRun);
    free(txTestRun);
}

void runAllTests(int sockfd, struct TestRunConfig *testRunConfig, struct TestRunManager *testRunManager) {
    struct FiveTuple *txFiveTuple;
    txFiveTuple = makeFiveTuple((struct sockaddr *)&listenConfig.localAddr,
                                (struct sockaddr *)&listenConfig.remoteAddr, listenConfig.port);

    sendStarOfTest(testRunConfig, txFiveTuple , sockfd);
    int numPkt = runTests(sockfd, txFiveTuple, testRunConfig, testRunManager);
    sendEndOfTest(testRunConfig, txFiveTuple, numPkt, sockfd);
    free(txFiveTuple);
}

int
main(int   argc,
     char* argv[])
{
    struct TestRunConfig testRunConfig;
    memset(&testRunConfig, 0, sizeof(testRunConfig));

    /* Read cmd line arguments and set it up */
    configure(&testRunConfig, &listenConfig,  argc, argv);

    /* Setting up UDP socket  */
    setupSocket(&listenConfig, &listenConfig);

    /* at least close the socket if we get a signal.. */
    signal(SIGINT, teardown);

    char              addrStr[SOCKADDR_MAX_STRLEN];
    printf( "Sending packets from: '%s'",
    sockaddr_toString( (struct sockaddr*)&listenConfig.localAddr,
           addrStr,
           sizeof(addrStr),
           false ) );

    printf( "to: '%s'\n",
    sockaddr_toString( (struct sockaddr*)&listenConfig.remoteAddr,
           addrStr,
           sizeof(addrStr),
                       true ) );

    struct TestRunManager testRunManager;
    initTestRunManager(&testRunManager);

    addTxAndRxTests(&testRunConfig, &testRunManager);
    int sockfd = startListenThread(&testRunManager, &listenConfig);
    runAllTests(sockfd, &testRunConfig, &testRunManager);
    printf("\n");
    waitForResponses(&testRunConfig, &testRunManager);
    pruneLingeringTestRuns(&testRunManager);
    freeTestRunManager(&testRunManager);
    return 0;
}




