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
#include <ctype.h>
#include <packettest.h>

#include "iphelper.h"
#include "sockethelper.h"


static struct ListenConfig listenConfig;

#define max_iface_len 10

#define MAX_DELAY_NS 999999999

struct client_config {
  char                    interface[10];
  struct sockaddr_storage localAddr;
  struct sockaddr_storage remoteAddr;
  int                     port;
  int                     dscp;
  struct TestRunConfig    testRunConfig;

};



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
printUsage()
{
  printf("Usage: stunclient [options] server\n");
  printf("Options: \n");
  printf("  -i, --interface           Interface\n");
  printf("  -p <port>, --port <port>  Destination port\n");
  printf("  -d <ms>, --delay <ms>     Delay after each sendto\n");
  printf("  -t <0xNN> --dscp <0xNN>   DSCP/Diffserv value\n");
  printf("  -v, --version             Print version number\n");
  printf("  -h, --help                Print help text\n");
  exit(0);
}


void
configure(struct client_config* config,
          int                   argc,
          char*                 argv[])
{
    int c;
    /* int                 digit_optind = 0; */
    /* set config to default values */
    strncpy(config->interface, "default", 7);
    config->port  = 3478;
    config->testRunConfig.numPktsToSend = 3000;
    config->testRunConfig.delay.tv_sec = 0;
    config->testRunConfig.delay.tv_nsec = 20000000L;
    config->testRunConfig.looseNthPkt = 0;
    config->dscp = 0;

    static struct option long_options[] = {
        {"interface", 1, 0, 'i'},
        {"port", 1, 0, 'p'},
        {"pkts", 1, 0, 'n'},
        {"delay", 1, 0, 'd'},
        {"dscp", 1, 0, 't'},
        {"help", 0, 0, 'h'},
        {"version", 0, 0, 'v'},
        {NULL, 0, NULL, 0}
    };
    if (argc < 2){
        printUsage();
        exit(0);
    }
    int option_index = 0;
    while ( ( c = getopt_long(argc, argv, "hvi:p:o:n:d:t:",
                            long_options, &option_index) ) != -1 )
    {
    /* int this_option_optind = optind ? optind : 1; */
    switch (c)
    {
        case 'i':
          strncpy(config->interface, optarg, max_iface_len);
          break;
        case 'p':
          config->port = atoi(optarg);
          break;
        case 'd':
            config->testRunConfig.delay.tv_nsec = atoi(optarg)*1000000L;
          break;
        case 't':
            config->dscp = strtoul(optarg, NULL, 16);
        case 'n':
            config->testRunConfig.numPktsToSend = atoi(optarg);
            if (config->testRunConfig.numPktsToSend > MAX_NUM_RCVD_TEST_PACKETS)
            {
                config->testRunConfig.numPktsToSend = MAX_NUM_RCVD_TEST_PACKETS;
            }
            break;
        case 'h':
          printUsage();
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
        if ( !getRemoteIpAddr( (struct sockaddr*)&config->remoteAddr,
                               argv[optind++],
                               config->port ) ){
          printf("Error getting remote IPaddr");
          exit(1);
        }
    }


  if ( !getLocalInterFaceAddrs( (struct sockaddr*)&config->localAddr,
                                config->interface,
                                config->remoteAddr.ss_family,
                                IPv6_ADDR_NORMAL,
                                false ) )
  {
    printf("Error getting IPaddr on %s\n", config->interface);
    exit(1);
  }

}


int
setupSocket(struct ListenConfig *lconf, const struct client_config* sconfig)
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
    struct client_config clientConfig;

    /* Read cmd line argumens and set it up */
    configure(&clientConfig,argc,argv);

    /* Setting up UDP socket  */
    setupSocket(&listenConfig, &clientConfig);

    /* at least close the socket if we get a signal.. */
    signal(SIGINT, teardown);

    char              addrStr[SOCKADDR_MAX_STRLEN];
    printf( "Sending packets from: '%s'",
    sockaddr_toString( (struct sockaddr*)&clientConfig.localAddr,
           addrStr,
           sizeof(addrStr),
           false ) );

    printf( "to: '%s'\n",
    sockaddr_toString( (struct sockaddr*)&clientConfig.remoteAddr,
           addrStr,
           sizeof(addrStr),
                       true ) );


    uint8_t buf[1200];
    memset(&buf, 43, sizeof(buf));

    //struct timespec timer;
    struct timespec remaining;
    //timer.tv_sec  = 0;
    //timer.tv_nsec = clientConfig.testRunConfig.delayns;

    struct TestRun testRun;
    initTestRun(&testRun, clientConfig.testRunConfig.numPktsToSend, clientConfig.testRunConfig);

    int sockfd = listenConfig.socketConfig[0].sockfd;

    for(int j=0, nth=1;j<testRun.config.numPktsToSend;j++,nth++){
      struct TestPacket pkt = getNextTestPacket(&testRun);
      memcpy(buf, &pkt, sizeof(pkt));
      if(nth == testRun.config.looseNthPkt){
          printf("Simulating pkt loss. Dropping packet (%i)\n", pkt.seq);
          nth=1;
      }else {
          sendPacket(sockfd, (const uint8_t *) &buf, sizeof(buf),
                     (const struct sockaddr *) &clientConfig.remoteAddr,
                             0,clientConfig.dscp,0);
      }
      if(j%100==0){
          printf("\r%i", j);
          fflush(stdout);
      }
      addTestData(&testRun, &pkt);
      nanosleep(&clientConfig.testRunConfig.delay, &remaining);
    }

    //Send End of Test (MAX_INT) a few times...
    struct TestPacket pkt = getEndTestPacket(&testRun);
    memcpy(buf, &pkt, sizeof(pkt));
    for(int j=0;j<10;j++){
      sendPacket(sockfd, (const uint8_t *)&buf, sizeof(buf), (const struct sockaddr*)&clientConfig.remoteAddr,
              0, clientConfig.dscp,0 );
      nanosleep(&clientConfig.testRunConfig.delay, &remaining);
    }

    const char* filename = "client_results.txt\0";
    saveTestDataToFile(&testRun, filename);
    freeTestRun(&testRun);

    return 0;
}


