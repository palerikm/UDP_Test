#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "fivetuple.h"


struct FiveTuple* makeFiveTuple(const struct sockaddr* src,
                                const struct sockaddr* dst,
                                int port){
    struct FiveTuple *fiveTuple;
    fiveTuple = (struct FiveTuple*)malloc(sizeof(struct FiveTuple));
    memset(fiveTuple, 0, sizeof(struct FiveTuple));
    sockaddr_copy((struct sockaddr *)&fiveTuple->src, src);
    sockaddr_copy((struct sockaddr *)&fiveTuple->dst, dst);
    fiveTuple->port = port;
    return fiveTuple;
}

bool fiveTupleAlike(const struct FiveTuple* a, const struct FiveTuple* b){
    if( ! sockaddr_alike((const struct sockaddr *)&a->src,
                         (const struct sockaddr *)&b->src)){
        return false;
    }

    if( ! sockaddr_alike((const struct sockaddr *)&a->dst,
                         (const struct sockaddr *)&b->dst)){
        return false;
    }

    return a->port != b->port ? false : true;
}


char  *fiveTupleToString(char *str, const struct FiveTuple *tuple){

    char              addrStr[SOCKADDR_MAX_STRLEN];
    sockaddr_toString( (struct sockaddr*)&tuple->src,
                       addrStr,
                       sizeof(addrStr),
                       false );
    strncpy(str, addrStr, sizeof(addrStr));
    strncat(str, " \0", 1);
    sockaddr_toString( (struct sockaddr*)&tuple->dst,
                       addrStr,
                       sizeof(addrStr),
                       false );
    strncat(str, addrStr, strlen(addrStr));
    char result[50];
    sprintf(result, " %i", tuple->port);
    strncat(str, result, strlen(result));
    return str;
}