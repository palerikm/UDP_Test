#include "JitterData.h"

int JitterData::getSeq() const {
    return seq;
}

void JitterData::setSeq(int seq) {
    JitterData::seq = seq;
}

double JitterData::getJitterMs() const {
    return jitter_ms;
}

void JitterData::setJitterMs(double jitterMs) {
    jitter_ms = jitterMs;
}

bool JitterData::isLostPkt() const {
    return lostPkt;
}

void JitterData::setLostPkt(bool lostPkt) {
    JitterData::lostPkt = lostPkt;
}

JitterData::JitterData(int id, int seq, double jitterMs, bool lostPkt) : id(id), seq(seq), jitter_ms(jitterMs), lostPkt(lostPkt) {}
