

#include "TestRunWorker.h"

#include <iostream>
#include <chrono>
#include <thread>

using namespace std;


TestRunWorker::TestRunWorker(QObject *parent) :
        QObject(parent)
{
    //mp = make_shared<MediaProcessing>(s);
    //pBackSub = createBackgroundSubtractorMOG2();
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



void TestRunWorker::receiveSetup()
{
    srand( (unsigned)time(NULL) );

    while(true){
        int i = (rand()%100)+1;
       // cout<<i<<endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        emit sendData(i);
    }
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

