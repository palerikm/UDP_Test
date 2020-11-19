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
  printf("  -t <0xNN> --dscp <0xNN>   DSCP/Diffserv value\n");
  printf("  -s <bytes> --size <bytes> Size of packet payload in bytes\n");
  printf("  -v, --version             Print version number\n");
  printf("  -h, --help                Print help text\n");
  exit(0);
}


void configure(struct TestRunConfig* config,
          int                   argc,
          char*                 argv[])
{
    int c;
    /* int                 digit_optind = 0; */
    /* set config to default values */
    strncpy(config->interface, "default", 7);
    config->port  = 3478;
    config->numPktsToSend = 3000;
    config->delay.tv_sec = 0;
    config->delay.tv_nsec = 20000000L;
    config->looseNthPkt = 10;
    config->dscp = 0;
    config->pkt_size = 1200;

    static struct option long_options[] = {
        {"interface", 1, 0, 'i'},
        {"port", 1, 0, 'p'},
        {"pkts", 1, 0, 'n'},
        {"delay", 1, 0, 'd'},
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
    while ( ( c = getopt_long(argc, argv, "hvi:p:o:n:d:t:s:",
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
            config->delay.tv_nsec = atoi(optarg)*1000000L;
          break;
        case 't':
            config->dscp = strtoul(optarg, NULL, 16);
        case 's':
            config->pkt_size = atoi(optarg);
            break;
        case 'n':
            config->numPktsToSend = atoi(optarg);
            if (config->numPktsToSend > MAX_NUM_RCVD_TEST_PACKETS)
            {
                config->numPktsToSend = MAX_NUM_RCVD_TEST_PACKETS;
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
setupSocket(struct ListenConfig *lconf, const struct TestRunConfig* sconfig)
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

    /* Read cmd line argumens and set it up */
    configure(&testRunConfig, argc, argv);

    /* Setting up UDP socket  */
    setupSocket(&listenConfig, &testRunConfig);

    /* at least close the socket if we get a signal.. */
    signal(SIGINT, teardown);

    char              addrStr[SOCKADDR_MAX_STRLEN];
    printf( "Sending packets from: '%s'",
    sockaddr_toString( (struct sockaddr*)&testRunConfig.localAddr,
           addrStr,
           sizeof(addrStr),
           false ) );

    printf( "to: '%s'\n",
    sockaddr_toString( (struct sockaddr*)&testRunConfig.remoteAddr,
           addrStr,
           sizeof(addrStr),
                       true ) );


    uint8_t buf[testRunConfig.pkt_size];
    memset(&buf, 43, sizeof(buf));

    struct timespec remaining;

    struct TestRun testRun;
    initTestRun(&testRun, testRunConfig.numPktsToSend, &testRunConfig);

    int sockfd = listenConfig.socketConfig[0].sockfd;

    //Send End of Test a few times...
    struct TestPacket pkt = getStartTestPacket(&testRun);
    memcpy(buf, &pkt, sizeof(pkt));

    for(int j=0;j<3;j++){
        sendPacket(sockfd, (const uint8_t *)&buf, sizeof(buf), (const struct sockaddr*)&testRunConfig.remoteAddr,
                   0, testRunConfig.dscp, 0 );
        addTestData(&testRun, &pkt);
        nanosleep(&testRunConfig.delay, &remaining);
    }


    for(int j=0, nth=1;j<testRun.config->numPktsToSend;j++,nth++){
      pkt = getNextTestPacket(&testRun);
      memcpy(buf, &pkt, sizeof(pkt));
      if(nth == testRun.config->looseNthPkt){
          printf("Simulating pkt loss. Dropping packet (%i)\n", pkt.seq);
          nth=1;
      }else {
          sendPacket(sockfd, (const uint8_t *) &buf, sizeof(buf),
                     (const struct sockaddr *) &testRunConfig.remoteAddr,
                     0, testRunConfig.dscp, 0);
      }
      if(j%100==0){
          printf("\r%i", j);
          fflush(stdout);
      }
      addTestData(&testRun, &pkt);
      nanosleep(&testRunConfig.delay, &remaining);
    }

    //Send End of Test a few times...
    pkt = getEndTestPacket(&testRun);
    memcpy(buf, &pkt, sizeof(pkt));
    for(int j=0;j<10;j++){
      sendPacket(sockfd, (const uint8_t *)&buf, sizeof(buf), (const struct sockaddr*)&testRunConfig.remoteAddr,
                 0, testRunConfig.dscp, 0 );
        addTestData(&testRun, &pkt);
      nanosleep(&testRunConfig.delay, &remaining);

    }

    const char* filename = "client_results.txt\0";
    saveTestDataToFile(&testRun, filename);
    freeTestRun(&testRun);

    return 0;
}


