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


void extractRespTestData(const unsigned char *buf, struct TestRun *run) {
    int32_t numTestData = 0;
    uint32_t currPosition = sizeof(struct TestPacket);
    memcpy(&numTestData, buf + currPosition, sizeof(int32_t));
    if(numTestData > 0){
    currPosition+= sizeof(int32_t);
    //if(run != NULL){
    struct TestData data;
    memcpy(&data, buf+currPosition, sizeof(struct TestData));
    if(data.pkt.pktCookie == TEST_PKT_COOKIE) {
        for(int i=0;i<numTestData;i++){
            memcpy(&data, buf+currPosition, sizeof(struct TestData));
            //printf("seq: %i ", data.pkt.seq);
            if(run->numTestData == 0) {
                memcpy(run->testData + run->numTestData, &data, sizeof(struct TestData));
                run->numTestData++;
                //currPosition+=sizeof(struct TestData);
            }else{
                struct TestData *prev = run->testData + run->numTestData -1;
                if(data.pkt.seq == prev->pkt.seq +1){
                    memcpy(run->testData + run->numTestData, &data, sizeof(struct TestData));
                    run->numTestData++;
                    //currPosition+=sizeof(struct TestData);
                }
            }
            currPosition+=sizeof(struct TestData);
        //    printf(" Resp Run contains %i \n ", run->numTestData);
        //    fflush(stdout);

        }
    }
    // }
    }
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

    int res = addTestDataFromBuf(mngr, tuple, buf, buflen, &now);

    if(res<0){
        printf("Encountered error while processing incoming UDP packet\n");
    }
    //lets check if there is more stuff in the packet..

    struct FiveTuple *txtuple = makeFiveTuple((const struct sockaddr *)&config->localAddr,
                          (const struct sockaddr *)&config->remoteAddr,
                          config->port);
    struct TestRun *run = findTestRun(mngr, txtuple);
    extractRespTestData(buf,  run);


}

void sendSomePktTest(struct TestRunConfig *cfg, const struct TestPacket *pkt, struct FiveTuple *fiveTuple, int sockfd) {
    //Send End of Test a few times...
    struct timespec now, remaining;
    uint8_t endBuf[cfg->pktConfig.pkt_size];
    memcpy(endBuf, pkt, sizeof(struct TestPacket));
    for(int j=0;j<10;j++){
        clock_gettime(CLOCK_MONOTONIC_RAW, &now);
        sendPacket(sockfd, (const uint8_t *)&endBuf, sizeof(endBuf),
                   (const struct sockaddr*)&fiveTuple->dst,
                   0, cfg->pktConfig.dscp, 0 );


        //addTestDataFromBuf(mng, fiveTuple,
        //                   endBuf, sizeof(endBuf), &now);

        nanosleep(&cfg->pktConfig.delay, &remaining);
    }
}

void sendEndOfTest(struct TestRunConfig *cfg, struct FiveTuple *fiveTuple, int num, int sockfd) {

    struct TestPacket pkt = getEndTestPacket(cfg->testName, num);


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

    struct TestPacket startPkt = getStartTestPacket(cfg->testName);

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
    listenConfig->port  = 3478;
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
            strncpy(config->testName, optarg, MAX_TESTNAME_LEN);
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
            if (config->pktConfig.numPktsToSend > MAX_NUM_RCVD_TEST_PACKETS)
            {
                config->pktConfig.numPktsToSend = MAX_NUM_RCVD_TEST_PACKETS;
            }
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


int
main(int   argc,
     char* argv[])
{
    struct TestRunConfig testRunConfig;
    memset(&testRunConfig, 0, sizeof(testRunConfig));

    struct timespec overshoot = {0,0};

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

    uint8_t buf[testRunConfig.pktConfig.pkt_size];
    memset(&buf, 43, sizeof(buf));

    struct timespec startOfTest, startBurst,endBurst, inBurst, timeBeforeSendPacket, timeLastPacket;

    //Do a quick test to check how "accurate" nanosleep is on this platform
    nap(&testRunConfig.pktConfig.delay, &overshoot);


    struct TestRunManager testRunManager;
   // memcpy(&testRunManager.defaultConfig, &testRunConfig, sizeof(struct TestRunConfig));
    testRunManager.map = hashmap_new(sizeof(struct TestRun), 0, 0, 0,
                                     TestRun_hash, TestRun_compare, NULL);



    struct FiveTuple *txFiveTuple;
    txFiveTuple = makeFiveTuple((struct sockaddr *)&listenConfig.localAddr,
                              (struct sockaddr *)&listenConfig.remoteAddr, listenConfig.port);
    struct TestRun txTestRun;
    //initTestRun(&results.txTestRun, testRunConfig.pktConfig.numPktsToSend, txFiveTuple, &testRunConfig);
    initTestRun(&txTestRun, testRunConfig.pktConfig.numPktsToSend, txFiveTuple, &testRunConfig);
    clock_gettime(CLOCK_MONOTONIC_RAW, &startBurst);
    txTestRun.lastPktTime = startBurst;
    txTestRun.stats.startTest = startBurst;
    hashmap_set(testRunManager.map, &txTestRun);

    struct FiveTuple *rxFiveTuple;
    rxFiveTuple = makeFiveTuple((struct sockaddr *)&listenConfig.remoteAddr,
                                (struct sockaddr *)&listenConfig.localAddr, listenConfig.port);
    //struct TestRunConfig rxConfig = testRunConfig;


    int sockfd = listenConfig.socketConfig[0].sockfd;


    pthread_t socketListenThread;
    listenConfig.pkt_handler          = packetHandler;
    listenConfig.tInst = &testRunManager;
    pthread_create(&socketListenThread,
                   NULL,
                   socketListenDemux,
                   (void*)&listenConfig);




    sendStarOfTest(&testRunConfig, txFiveTuple, sockfd);


    struct TestPacket pkt;

    int currBurstIdx = 0;
    clock_gettime(CLOCK_MONOTONIC_RAW, &startBurst);
    startOfTest = startBurst;

    int numPkt  = 0;
    struct TestRunResponse r;
    for(numPkt=0 ;numPkt<testRunConfig.pktConfig.numPktsToSend;numPkt++){

        struct TestRun *run = findTestRun(&testRunManager, txFiveTuple);
        if(run != NULL) {
            r.lastIdxConfirmed = run->numTestData;
        }

        clock_gettime(CLOCK_MONOTONIC_RAW, &timeBeforeSendPacket);
        struct timespec timeSinceLastPkt;
        timespec_sub(&timeSinceLastPkt, &timeBeforeSendPacket, &timeLastPacket);
        //Get next test packet to send.
        //pkt = getNextTestPacket(testRun, &timeBeforeSendPacket);
        fillPacket(&pkt, 23, numPkt+1, in_progress_test_cmd,
                   &timeSinceLastPkt, &r, NULL);
        memcpy(buf, &pkt, sizeof(pkt));



        sendPacket(sockfd, (const uint8_t *) &buf, sizeof(buf),
                 (const struct sockaddr *) &txFiveTuple->dst,
                 0, testRunConfig.pktConfig.dscp, 0);

        //addTestDataFromBuf(&testRunManager, testRun->fiveTuple,
        //                   buf, sizeof(buf), &timeBeforeSendPacket);

        printStats(&testRunManager, &timeBeforeSendPacket, &startOfTest, numPkt, testRunConfig.pktConfig.numPktsToSend, testRunConfig.pktConfig.pkt_size );


        //Do I sleep or am I bursting..
        if(currBurstIdx < testRunConfig.pktConfig.pktsInBurst){
          currBurstIdx++;
        }else {
          currBurstIdx = 0;
          struct timespec delay = {0,0};
          clock_gettime(CLOCK_MONOTONIC_RAW, &endBurst);
            timespec_sub(&inBurst, &endBurst, &startBurst);
          delay.tv_sec = testRunConfig.pktConfig.delay.tv_sec - inBurst.tv_sec - overshoot.tv_sec;
          delay.tv_nsec = testRunConfig.pktConfig.delay.tv_nsec - inBurst.tv_nsec - overshoot.tv_nsec;
          nap(&delay, &overshoot);
          //Staring a new burst when we start at top of for loop.
          clock_gettime(CLOCK_MONOTONIC_RAW, &startBurst);
        }
        timeLastPacket = timeBeforeSendPacket;
    }//End of main test run. Just some cleanup remaining.

    sendEndOfTest(&testRunConfig, txFiveTuple, numPkt, sockfd);

    printf("\n");


    bool done = false;
    char fileEnding[] = "_testrun.txt\0";

    //Ish, A bit hacky to get different names when saving.. (TODO:Fixit?)
    struct TestRun *txrun = findTestRun(&testRunManager, txFiveTuple);
    strncat(txrun->config.testName, "_tx\0", 5);
    txrun->done = true;

    struct TestRun *rxrun = findTestRun(&testRunManager, rxFiveTuple);
    strncat(rxrun->config.testName, "_rx\0", 5);
    
   while(!done){
        int num = getNumberOfActiveTestRuns(&testRunManager);
        if(  num > 0 ){
            saveAndDeleteFinishedTestRuns(&testRunManager, fileEnding);
            nap(&testRunConfig.pktConfig.delay, &overshoot);
        }else{
            done = true;
        }

    }

    free(rxFiveTuple);
    free(txFiveTuple);
    pruneLingeringTestRuns(&testRunManager);
    freeTestRunManager(&testRunManager);

    return 0;
}




