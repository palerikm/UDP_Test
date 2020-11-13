#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <inttypes.h>

#include <pthread.h>
#include <getopt.h>

#include <stdbool.h>
#include <packettest.h>

#include "../include/iphelper.h"
#include "../include/sockethelper.h"

#define MAXBUFLEN 1500
#define MAX_LISTEN_SOCKETS 1
#define MAX_THREADS 100000

#define max_iface_len 10
int             sockfd;


uint64_t timespec_subtract(struct timespec *t2, struct timespec *t1)
{
    return ((t1->tv_sec - t2->tv_sec) * 1000000 + (t1->tv_nsec - t2->tv_nsec))/1000;
}
static inline void timespec_diff(struct timespec *a, struct timespec *b,
                                 struct timespec *result) {
    result->tv_sec  = a->tv_sec  - b->tv_sec;
    result->tv_nsec = a->tv_nsec - b->tv_nsec;
    if (result->tv_nsec < 0) {
        --result->tv_sec;
        result->tv_nsec += 1000000000L;
    }
}


void
packetHandler(struct SocketConfig* config,
            struct sockaddr*     from_addr,
            void*                cb,
            unsigned char*       buf,
            int                  buflen) {
    struct timespec begin, result;
    clock_gettime(CLOCK_MONOTONIC_RAW, &begin);
    struct TestData *testData = config->testData;

    //gettimeofday(&tv2, NULL);
    struct TestPacket pkt;
    memcpy(&pkt, buf, sizeof(pkt));
    if(pkt.pktCookie != TEST_PKT_COOKIE){
        printf("Not a test pkt! Ignoring\n");
        return;
    }

    if( config->currentPktNum == UINT32_MAX && pkt.seq == UINT32_MAX){
        //printf("Already recieved end of test. Ignoring..\n");
        return;
    }
    //uint64_t diff = timespec_subtract(&config->currentPktTimeVal,&begin );
    timespec_diff(&begin, &config->currentPktTimeVal, &result);

    if( config->currentPktNum+1!= pkt.seq){
        printf("Lost Packet(s): %i\n", config->currentPktNum - pkt.seq);
    }
    struct TestData d;
    d.pkt = pkt;
    d.timeDiff = result.tv_nsec;
    d.lostPkts = config->currentPktNum - pkt.seq;

    memcpy((testData+config->numTestData), &d, sizeof(struct TestData));
    config->numTestData++;

    config->currentPktNum = pkt.seq;
    config->currentPktTimeVal = begin;
#if 0
    if(pkt.seq > 1) {

        if(pkt.seq%100==0) {
            printf("\r%i  ", pkt.seq);
            //printf("       %ld.%06ld\n", diff.tv_sec, diff.tv_nsec);
            printf("    Diff:  %ld\n", result.tv_nsec);


            fflush(stdout);
        }
    }
#endif

}

static void*
saveAndMoveOn(void* ptr)
{
    struct SocketConfig *config = ptr;

    for (;; )
    {
        usleep(5000);
        if(config->numTestData < 4){
            usleep(10000);
            continue;
        }
        struct TestData *end;
        end = config->testData+config->numTestData-1;
        //Opps, Hopefully UINT32_MAX is not to different on the various targes..
        //TODO Fix..
        if(end->pkt.seq == UINT32_MAX){
            printf("Saving--- (%i)\n", config->numTestData);
            FILE *fptr;
            fptr = fopen("data.txt","w");
            if(fptr == NULL)
            {
                printf("Error!");
                exit(1);
            }
            for(int i=1; i<config->numTestData-1;i++){
                struct TestData *muh;
                muh = config->testData+i;
                fprintf(fptr, "%i,%ld\n", muh->pkt.seq, muh->timeDiff);
            }
            fclose(fptr);
            config->numTestData = 0;
            printf("----Done saving\n");
        }
    }
    return NULL;
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
    struct sockaddr_storage localAddr;
    char                    interface[10];
    int                     port;



    pthread_t socketListenThread;
    pthread_t cleanupThread;

    struct ListenConfig listenConfig;

    listenConfig.socketConfig->testData = malloc(sizeof(struct TestData)*MAX_NUM_RCVD_TEST_PACKETS);
    listenConfig.socketConfig->numTestData = 0;


    signal(SIGINT, teardown);
    int c;
    /* int                 digit_optind = 0; */
    /* set config to default values */
    strncpy(interface, "default", 7);
    port          = 3478;


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
                strncpy(interface, optarg, max_iface_len);
                break;
            case 'p':
                port = atoi(optarg);
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

    if ( !getLocalInterFaceAddrs( (struct sockaddr*)&localAddr,
                                  interface,
                                  AF_INET,
                                  IPv6_ADDR_NORMAL,
                                  false ) )
    {
        printf("Error getting IPaddr on %s\n", interface);
        exit(1);
    }

    sockfd = createLocalSocket(AF_INET,
                               (struct sockaddr*)&localAddr,
                               SOCK_DGRAM,
                               port);

    listenConfig.socketConfig[0].sockfd = sockfd;
    listenConfig.pkt_handler          = packetHandler;
    listenConfig.numSockets             = 1;



    pthread_create(&socketListenThread,
                   NULL,
                   socketListenDemux,
                   (void*)&listenConfig);
    pthread_create(&cleanupThread, NULL, saveAndMoveOn, (void*)&listenConfig.socketConfig);
    pause();

}

