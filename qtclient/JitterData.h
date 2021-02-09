//
// Created by Pal-Erik Martinsen on 09/02/2021.
//

#ifndef UDP_TESTS_JITTERDATA_H
#define UDP_TESTS_JITTERDATA_H


class JitterData {
public:
    JitterData(int id, int seq, double jitterMs, bool lostPkt);

    int getSeq() const;

    void setSeq(int seq);

    double getJitterMs() const;

    void setJitterMs(double jitterNs);

    bool isLostPkt() const;

    void setLostPkt(bool lostPkt);

    bool cmpJitter(const JitterData &a, const JitterData &b);

    int id;
    int seq;
    double jitter_ms;
    bool lostPkt;
private:

};


#endif //UDP_TESTS_JITTERDATA_H
