#ifndef UDP_TESTS_IPHELPER_H
#define UDP_TESTS_IPHELPER_H
#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>

typedef enum IPv6_ADDR_TYPES {IPv6_ADDR_NONE,
    IPv6_ADDR_ULA,
    IPv6_ADDR_PRIVACY,
    IPv6_ADDR_NORMAL} IPv6_ADDR_TYPE;


bool
getLocalInterFaceAddrs(struct sockaddr* addr,
                       char*            iface,
                       int              ai_family,
                       IPv6_ADDR_TYPE   ipv6_addr_type,
                       bool             force_privacy);

int
getRemoteIpAddr(struct sockaddr* remoteAddr,
                const char*            fqdn,
                uint16_t         port);

#ifdef __cplusplus
}
#endif
#endif //UDP_TESTS_IPHELPER_H
