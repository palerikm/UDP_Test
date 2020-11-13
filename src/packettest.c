//
// Created by Pal-Erik Martinsen on 12/11/2020.
//

#include "packettest.h"


uint32_t fillPacket(struct TestPacket *testPacket, uint32_t srcId, uint32_t seq){
    testPacket->pktCookie = TEST_PKT_COOKIE;
    testPacket->srcId = srcId;
    testPacket->seq = seq;
    return 0;
}