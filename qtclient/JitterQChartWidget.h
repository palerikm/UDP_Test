#pragma once


#include <QWidget>
#include <QList>
#include <QSplineSeries>
#include <QScatterSeries>
#include <QChart>
#include <QChartView>

#include "ControlWindow.h"

using namespace QtCharts;

namespace Ui {
    class JitterQChartWidget;
}

class JitterQChartWidget : public QWidget
{
Q_OBJECT

private:
    Ui::JitterQChartWidget *ui;

    ControlWindow *ctrlWindow;

    QThread *thread;

    int timerId;
    int maxSize; // txData stores at most maxSize elements
    int maxX;
    int maxY;
    int nth;
    QList<double> txData; // A list to store business txData
    QList<double> rxData; // A list to store business txData
    QChart *txChart;
    QChart *rxChart;
    QChartView *txChartView;
    QChartView *rxChartView;

    QLineSeries *txLineSeries;
    QLineSeries *rxLineSeries;
    //QScatterSeries *scatterSeries;


    int lcdNth;



    //void setup(struct TestRunConfig *tConfig, struct ListenConfig *listenConfig);

public:
    explicit JitterQChartWidget(QWidget *parent = 0, struct TestRunConfig *tConfig = nullptr, struct ListenConfig *listenConfig = nullptr);

    ~JitterQChartWidget();

protected:
    //  void resizeEvent(QResizeEvent* event) override;



signals:
    void sendStartTestWorker();
    void sendStopTestWorker();


private slots:
    void receiveData(int, unsigned int, long);
    void updatePktStatus(double mbs, double ps);

    void startTest(struct TestRunConfig *tConfig, struct ListenConfig *listenConfig);
    void stopTest();

    //void receiveFrame(QImage frame);

    //void receiveProcessedFrame(QImage frame1, QImage frame2, QImage frame3);

    //void receiveActualFps(int actualFps);

    //oid receiveReadFrameTime(int time);

    //void receiveProcessTime(int time);
};
