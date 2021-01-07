#pragma once

#include <QObject>
#include <QImage>

#include "packettest.h"
#include "sockethelper.h"



class TestRunWorker : public QObject {
Q_OBJECT

private:
    struct TestRunConfig testRunConfig;
    struct ListenConfig listenConfig;
    struct TestRunManager testRunManager;

    void testRunDataCb(int i, uint32_t, int64_t);

public:
    explicit TestRunWorker(QObject *parent = nullptr,
                           struct TestRunConfig *tConfig = nullptr,
                           struct ListenConfig *listenConfig = nullptr  );


    ~TestRunWorker() override;

signals:

   void sendData(int, unsigned int, long);

    //void sendProcessedFrame(QImage frame1, QImage frame2, QImage frame3);

    //void sendActualFps(int actualFps);

    //void sendFrameReadTime(int time);

    //void sendProcessTime(int time);


public slots:

    //void receiveGrabFrame();

    void startTests();
};