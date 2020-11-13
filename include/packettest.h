//
// Created by Pal-Erik Martinsen on 12/11/2020.
//

#ifndef UDP_TESTS_PACKETTEST_H
#define UDP_TESTS_PACKETTEST_H

#define TEST_PKT_COOKIE 0x0023

#include <stdint.h>

struct TestPacket{
    uint32_t pktCookie;
    uint32_t srcId;
    uint32_t seq;
};

struct TestData{
    struct TestPacket pkt;
    long timeDiff;
    uint32_t lostPkts;
};

uint32_t fillPacket(struct TestPacket *testPacket, uint32_t srcId, uint32_t seq);

#endif //UDP_TESTS_PACKETTEST_H
