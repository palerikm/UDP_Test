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


struct SocketConfig {
    int   sockfd;
    uint32_t currentPktNum;
    struct timespec  currentPktTimeVal;
    double sumPktInterval;
    double maxPktInterval;
    double minPktInterval;
};





struct listenConfig {
    void*               tInst;
    struct SocketConfig socketConfig[MAX_LISTEN_SOCKETS];
    int                 numSockets;

    void (* pkt_handler)(struct SocketConfig*,
                          struct sockaddr*,
                          void*,
                          unsigned char*,
                          int);
    pthread_t           threads[MAX_THREADS];
    int                 thread_no;
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
           uint8_t                ttl);
#endif //UDP_TESTS_SOCKETHELPER_H
