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
    struct TestRun *testRun = config->tInst;

    addTestDataFromBuf(testRun, buf, buflen);
}

_Noreturn static void*
saveAndMoveOn(void* ptr)
{
    struct TestRun *testRun = ptr;
    const char* filename = "server_results.txt\0";
    for (;; )
    {
        if(testRun->done && testRun->numTestData > 4){
            saveTestDataToFile(testRun, filename);
            testRunReset(testRun);


        }else{
            usleep(10000);
        }
    }
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

    struct TestRun testRun;

    initTestRun(&testRun, MAX_NUM_RCVD_TEST_PACKETS, &testConfig);

    listenConfig.tInst = &testRun;

    pthread_create(&socketListenThread,
                   NULL,
                   socketListenDemux,
                   (void*)&listenConfig);
    pthread_create(&cleanupThread, NULL, saveAndMoveOn, (void*)&testRun);
    pause();

}

