#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <pthread.h>
#include <getopt.h>



#include <string.h>
#include <ctype.h>

#include "iphelper.h"
#include "sockethelper.h"
static struct listenConfig listenConfig;


#define max_iface_len 10
#define MAX_TRANSACTIONS 60

struct client_config {
  char                    interface[10];
  struct sockaddr_storage localAddr;
  struct sockaddr_storage remoteAddr;
  int                     port;
  int                     jobs;
};
static struct client_config config;

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
  printf(
    "  -j <num>, --jobs <num>    Run <num> transactions in paralell(almost)\n");
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
  config->jobs  = 1;
//  config->debug = false;


  static struct option long_options[] = {
    {"interface", 1, 0, 'i'},
    {"port", 1, 0, 'p'},
    {"jobs", 1, 0, 'j'},
    {"debug", 0, 0, 'd'},
    {"csv", 0, 0, '2'},
    {"help", 0, 0, 'h'},
    {"version", 0, 0, 'v'},
    {NULL, 0, NULL, 0}
  };
  if (argc < 2)
  {
    printUsage();
    exit(0);
  }
  int option_index = 0;
  while ( ( c = getopt_long(argc, argv, "hvdi:p:j:o:",
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
    //  config->debug = true;
      break;
    case 'j':
      config->jobs = atoi(optarg);
      if (config->jobs > MAX_TRANSACTIONS)
      {
        config->jobs = MAX_TRANSACTIONS;
      }
      break;
    case 'h':
      printUsage();
      break;
    case 'v':
      printf("Version \n");
      exit(0);
      break;
    default:
      printf("?? getopt returned character code 0%o ??\n", c);
    }
  }
  if (optind < argc)
  {
    if ( !getRemoteIpAddr( (struct sockaddr*)&config->remoteAddr,
                           argv[optind++],
                           config->port ) )
    {
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
setupSocket(struct client_config* config)
{
  for (int i = 0; i < config->jobs; i++)
  {
    int sockfd = createLocalSocket(config->remoteAddr.ss_family,
                                   (struct sockaddr*)&config->localAddr,
                                   SOCK_DGRAM,
                                   0);

    listenConfig.socketConfig[i].sockfd = sockfd;
    listenConfig.numSockets++;
  }
  return listenConfig.numSockets;
}



int
main(int   argc,
     char* argv[])
{

  //pthread_t socketListenThread;

  char              addrStr[SOCKADDR_MAX_STRLEN];

  /* Read cmd line argumens and set it up */
  configure(&config,argc,argv);

  /* Initialize STUNclient data structures */
  //StunClient_Alloc(&clientData);

  /* Setting up UDP socket and and aICMP sockhandle */
  setupSocket(&config);

  /* at least close the socket if we get a signal.. */
  signal(SIGINT, teardown);



  //pthread_create(&socketListenThread,
  //               NULL,
  //               socketListenDemux,
  //               (void*)&listenConfig);

    printf( "Sending binding  %i Req(s) from: '%s'",
            config.jobs,
            sockaddr_toString( (struct sockaddr*)&config.localAddr,
                               addrStr,
                               sizeof(addrStr),
                               true ) );

    printf( "to: '%s'\n",
            sockaddr_toString( (struct sockaddr*)&config.remoteAddr,
                               addrStr,
                               sizeof(addrStr),
                               true ) );

  for (int i = 0; i < listenConfig.numSockets; i++)
  {
      uint8_t buf[1200];
      memset(&buf, 0, sizeof(buf));
      uint32_t pktCnt = 0;
      struct timespec begin, end;
      int sockfd = listenConfig.socketConfig[i].sockfd;
      for(int i=0;i<500000;i++){

          pktCnt++;
          memcpy(buf, &pktCnt, sizeof(pktCnt));

          sendPacket(sockfd, (const uint8_t *)&buf, sizeof(buf), (const struct sockaddr*)&config.remoteAddr, 0, 0 );

          usleep(1000);

      }
  }
  return 0;
}
