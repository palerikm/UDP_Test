

#include "TestRunWorker.h"

#include <iostream>
#include <chrono>
#include <thread>

#include "clientcommon.h"


using namespace std;
template <typename T>
struct Callback;

template <typename Ret, typename... Params>
struct Callback<Ret(Params...)> {
    template <typename... Args>
    static Ret callback(Args... args) {
        return func(args...);
    }
    static std::function<Ret(Params...)> func;
};


template <typename Ret, typename... Params>
std::function<Ret(Params...)> Callback<Ret(Params...)>::func;

typedef void (*callback_t)(int, uint32_t, int64_t);

TestRunWorker::TestRunWorker(QObject *parent,
                             struct TestRunConfig *tConfig,
                                     struct ListenConfig *listenConfig) :
        QObject(parent)
{
    memcpy(&testRunConfig, tConfig, sizeof(testRunConfig));


    Callback<void(int, uint32_t, int64_t)>::func = std::bind(&TestRunWorker::testRunDataCb, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    callback_t func = static_cast<callback_t>(Callback<void(int, uint32_t, int64_t)>::callback);

    testRunConfig.TestRun_live_cb = func;
   memcpy( &this->listenConfig, listenConfig, sizeof(struct ListenConfig));
    /* Setting up UDP socket  */
    setupSocket(&this->listenConfig, &this->listenConfig);

    initTestRunManager(&testRunManager);

}

TestRunWorker::~TestRunWorker()
{
   // mp = nullptr;
}


//void OpenCvWorker::process() {
//    pBackSub->apply(originalFrame, processedFrame_1);
//    cv::flip(originalFrame, processedFrame_2, 1);
//    processedFrame_3 = originalFrame.clone();
//}

void TestRunWorker::testRunDataCb(int i, uint32_t seq, int64_t jitter)
{
    //cout<<"TestRun: "<<i<<"  seg:"<<seq<<"  jitter: "<<jitter<<endl;
    emit sendData(i, seq, jitter);
}


void TestRunWorker::startTests()
{
    addTxAndRxTests(&testRunConfig, &testRunManager, &listenConfig);
    int sockfd = startListenThread(&testRunManager, &listenConfig);
    runAllTests(sockfd, &testRunConfig, &testRunManager, &listenConfig);

    srand( (unsigned)time(NULL) );


}

//void OpenCvWorker::receiveGrabFrame() {
//    auto start = std::chrono::high_resolution_clock::now();
//    if (mp->readFrame(originalFrame)) {
//
//       auto stopReadFrame = std::chrono::high_resolution_clock::now();
//        auto durationReadFrame = std::chrono::duration_cast<std::chrono::microseconds>(stopReadFrame - start);
//
//        process();
//
//        auto stopProcess = std::chrono::high_resolution_clock::now();
//        auto durationProcess = std::chrono::duration_cast<std::chrono::microseconds>(stopProcess - stopReadFrame);
//
//        QImage outputOriginal(originalFrame.data, originalFrame.cols, originalFrame.rows, originalFrame.step,
//                              originalFrameFormat);
//        QImage outputProcessed_1(processedFrame_1.data, processedFrame_1.cols, processedFrame_1.rows,
//                                 processedFrame_1.step,
//                                 processedFrame_1Format);

//        QImage outputProcessed_2(processedFrame_2.data, processedFrame_2.cols, processedFrame_2.rows,
//                                 processedFrame_2.step,
//                                 processedFrame_2Format);

//        QImage outputProcessed_3(processedFrame_3.data, processedFrame_3.cols, processedFrame_3.rows,
//                                 processedFrame_3.step,
//                                 processedFrame_3Format);
//

//        emit sendFrame(outputOriginal);
//        emit sendProcessedFrame(outputProcessed_1, outputProcessed_2, outputProcessed_3);
//        emit sendActualFps(mp->getActualFps());
//        emit sendFrameReadTime(durationReadFrame.count() / 1000);
//        emit sendProcessTime(durationProcess.count() / 1000);
//    }

//}

