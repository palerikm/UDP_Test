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

#include <pthread.h>
#include <getopt.h>

#include <stdbool.h>

#include "../include/iphelper.h"
#include "../include/sockethelper.h"

#define MAXBUFLEN 1500
#define MAX_LISTEN_SOCKETS 1
#define MAX_THREADS 100000

#define max_iface_len 10
int             sockfd;



void
packetHandler(struct SocketConfig* config,
            struct sockaddr*     from_addr,
            void*                cb,
            unsigned char*       buf,
            int                  buflen) {
    struct timespec begin;
    clock_gettime(CLOCK_MONOTONIC_RAW, &begin);


    //gettimeofday(&tv2, NULL);
    uint32_t pktCnt = 0;
    memcpy(&pktCnt, buf, sizeof(pktCnt));

    double dur = (( begin.tv_nsec - config->currentPktTimeVal.tv_nsec ) / 1000000000.0 +
                  (   begin.tv_sec - config->currentPktTimeVal.tv_sec  )) *1000;

    if(dur<0){
        printf("negative duration!\n");
    }

    if( config->currentPktNum+1!= pktCnt){
        printf("Lost Packet!\n");
    }

    config->currentPktNum = pktCnt;
    config->currentPktTimeVal = begin;
    if(pktCnt > 1) {
        config->sumPktInterval += dur;
        if (config->maxPktInterval < dur) {
            config->maxPktInterval = dur;
        }
        if (config->minPktInterval > dur) {
            config->minPktInterval = dur;
        }
        if(pktCnt%100==0) {
            printf("\r%i  %f %f", pktCnt, config->sumPktInterval / config->currentPktNum,
                   config->maxPktInterval);
            fflush(stdout);
        }
    }
    //printf("Got som data(%i): %s\n", buflen, buf);
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


    struct listenConfig listenConfig;


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
    pause();

}

