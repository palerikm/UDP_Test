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

#define MAXBUFLEN 1500
#define MAX_LISTEN_SOCKETS 1
#define MAX_THREADS 100000


int             sockfd;


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
    //tuple = (struct FiveTuple*)malloc(sizeof(struct FiveTuple));
    //memset(tuple, 0, sizeof(struct FiveTuple));

    tuple = makeFiveTuple((const struct sockaddr *)&mng->defaultConfig.localAddr,
                  from_addr,
                  sockaddr_ipPort(from_addr));
    addTestDataFromBuf(mng, tuple, buf, buflen, &now);

    free(tuple);
/*

    struct FiveTuple tuple;

    makeFiveTuple(&tuple,
                  (const struct sockaddr *)&mng->defaultConfig.localAddr,
                  from_addr,
                  sockaddr_ipPort(from_addr));
    addTestDataFromBuf(mng, &tuple, buf, buflen, &now);
    */
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
        saveAndDeleteFinishedTestRuns(mngr, filenameEnding);
        pruneLingeringTestRuns(mngr);

        printf("\r Running Tests: %i ", getNumberOfActiveTestRuns(mngr));
        printf(" Mbps : %f ", getActiveBwOnAllTestRuns(mngr)/1000000);
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
    strncpy(testConfig.interface, "default", 7);
    testConfig.port          = 3478;


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
                strncpy(testConfig.interface, optarg, max_iface_len);
                break;
            case 'p':
                testConfig.port = atoi(optarg);
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

    if ( !getLocalInterFaceAddrs( (struct sockaddr*)&testConfig.localAddr,
                                  testConfig.interface,
                                  AF_INET,
                                  IPv6_ADDR_NORMAL,
                                  false ) )
    {
        printf("Error getting IPaddr on %s\n", testConfig.interface);
        exit(1);
    }

    sockfd = createLocalSocket(AF_INET,
                               (struct sockaddr*)&testConfig.localAddr,
                               SOCK_DGRAM,
                               testConfig.port);

    listenConfig.socketConfig[0].sockfd = sockfd;
    listenConfig.pkt_handler          = packetHandler;
    listenConfig.numSockets             = 1;

    struct TestRunManager testRunManager;
    testRunManager.defaultConfig = testConfig;
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

