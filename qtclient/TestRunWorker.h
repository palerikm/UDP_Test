#pragma once

#include <QObject>
#include <QImage>

#include "../udpjitterlib/include/udpjitter.h"
#include "sockethelper.h"



class TestRunWorker : public QObject {
Q_OBJECT

private:
    struct TestRunConfig *testRunConfig;
    struct ListenConfig *listenConfig;
    struct TestRunManager testRunManager;

    void testRunDataCb(int i, uint32_t, int64_t);
    void testRunStatusCB(double mpbs, double ps);

public:
    explicit TestRunWorker(QObject *parent = nullptr,
                           struct TestRunConfig *tConfig = nullptr,
                           struct ListenConfig *listenConfig = nullptr  );


    ~TestRunWorker() override;

signals:

   void sendData(int, unsigned int, long);
   void sendPktStatus(double, double );
   void finished();


public slots:
    void startTests();
    void stopTests();
};