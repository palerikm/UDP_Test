#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
#include <getopt.h>
#include <string.h>


#include <udpjitter.h>
#include <iphelper.h>
#include <sockethelper.h>

#include "clientcommon.h"


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
  printf("  -b <num>, --burst <num>   Packets to send in each burst\n");
  printf("  -t <0xNN> --tos <0xNN>   DSCP/Diffserv value\n");
  printf("  -s <bytes> --size <bytes> Size of packet payload in bytes\n");
  printf("  -v, --version             Print version number\n");
  printf("  -h, --help                Print help text\n");
  exit(0);
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
    config->pktConfig.tos = 0;
    config->pktConfig.pkt_size = 1200;


    int iseed = (unsigned int)time(NULL);
    srand(iseed);

    memset(config->testName, 0, MAX_TESTNAME_LEN);
    static const int a='a', z='z';
    for(int i=0; i<MAX_TESTNAME_LEN-10; i++) {
        config->testName[i] = rand() % (z - a + 1) + a;
    }
    config->testName[MAX_TESTNAME_LEN-10] = '\0';

    static struct option long_options[] = {
        {"interface", 1, 0, 'i'},
        {"name", 1, 0, 'w'},
        {"port", 1, 0, 'p'},
        {"pkts", 1, 0, 'n'},
        {"delay", 1, 0, 'd'},
        {"burst", 1, 0, 'b'},
        {"tos", 1, 0, 't'},
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
            config->pktConfig.tos = strtoul(optarg, NULL, 16);
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
main(int   argc,
     char* argv[])
{
    struct TestRunConfig testRunConfig;
    memset(&testRunConfig, 0, sizeof(testRunConfig));

    /* Read cmd line arguments and set it up */
    configure(&testRunConfig, &listenConfig,  argc, argv);

    /* Setting up UDP socket  */
    setupSocket(&listenConfig);

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

    struct TestRunManager *testRunManager = newTestRunManager(NULL);

    addTxAndRxTests(&testRunConfig, testRunManager, &listenConfig, NULL, NULL, true);
    int sockfd = startListenThread(testRunManager, &listenConfig);
    if ( runAllTests(sockfd, &testRunConfig, testRunManager, &listenConfig) != -1){
        printf("\n");
        //waitForResponses(&testRunConfig, testRunManager);
        pruneLingeringTestRuns(testRunManager);
        freeTestRunManager(&testRunManager);
    }else{
        printf("\nNo response from server!");
        pruneLingeringTestRuns(testRunManager);
        freeTestRunManager(&testRunManager);
        exit(1);
    }


    return 0;
}




