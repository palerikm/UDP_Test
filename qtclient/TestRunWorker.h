#pragma once

#include <QObject>
#include <QImage>





class TestRunWorker : public QObject {
Q_OBJECT

private:
    //std::shared_ptr<MediaProcessing> mp;

    //cv::Ptr<cv::BackgroundSubtractor> pBackSub;
    //cv::Mat originalFrame;
    //QImage::Format originalFrameFormat = QImage::Format_BGR888;
    //cv::Mat processedFrame_1;
    //QImage::Format processedFrame_1Format = QImage::Format_Grayscale8;
    //cv::Mat processedFrame_2;
    //QImage::Format processedFrame_2Format = QImage::Format_BGR888;
    //cv::Mat processedFrame_3;
    //QImage::Format processedFrame_3Format = QImage::Format_BGR888;

    //void process();

public:
    explicit TestRunWorker(QObject *parent = nullptr);

    ~TestRunWorker() override;

signals:

   void sendData(int value);

    //void sendProcessedFrame(QImage frame1, QImage frame2, QImage frame3);

    //void sendActualFps(int actualFps);

    //void sendFrameReadTime(int time);

    //void sendProcessTime(int time);


public slots:

    //void receiveGrabFrame();

    void receiveSetup();
};