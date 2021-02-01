#ifndef UDP_TESTS_FIVETUPLE_H
#define UDP_TESTS_FIVETUPLE_H
#ifdef __cplusplus
extern "C"
{
#endif


#include <sockaddrutil.h>

struct FiveTuple{
    struct sockaddr_storage src;
    struct sockaddr_storage dst;
    uint16_t                port;
};



struct FiveTuple* makeFiveTuple(const struct sockaddr* from_addr,
                                const struct sockaddr* to_addr,
                                int port);

char  *fiveTupleToString(char *str, const struct FiveTuple *tuple);


#ifdef __cplusplus
}
#endif
#endif //UDP_TESTS_FIVETUPLE_H
