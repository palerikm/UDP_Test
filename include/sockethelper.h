#ifndef UDP_TESTS_SOCKETHELPER_H
#define UDP_TESTS_SOCKETHELPER_H
#include <sockaddr_util.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>

#define MAXBUFLEN 1500
#define MAX_LISTEN_SOCKETS 1
#define MAX_THREADS 100000
#define MAX_NUM_RCVD_TEST_PACKETS 500000

#define max_iface_len 10

struct SocketConfig {
    int   sockfd;
};

struct ListenConfig {
    void*               tInst;
    struct SocketConfig socketConfig[MAX_LISTEN_SOCKETS];
    int                 numSockets;
    struct sockaddr_storage localAddr;
    struct sockaddr_storage remoteAddr;
    int                      port;
    char                    interface[10];

    void (* pkt_handler)(struct ListenConfig*,
                          struct sockaddr*,
                          void*,
                          unsigned char*,
                          int);
};

int
createLocalSocket(int                    ai_family,
                  const struct sockaddr* localIp,
                  int                    ai_socktype,
                  uint16_t               port);


void*
socketListenDemux(void* ptr);

void
sendPacket(int                    sockHandle,
           const uint8_t*         buf,
           int                    bufLen,
           const struct sockaddr* dstAddr,
           int                    proto,
           int                    dscp,
           uint8_t                ttl);
#endif //UDP_TESTS_SOCKETHELPER_H
