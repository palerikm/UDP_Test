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


class JitterData{

public:
    JitterData(int seq, double jitterMs, bool lostPkt);

    int getSeq() const;

    void setSeq(int seq);

    double getJitterMs() const;

    void setJitterMs(double jitterNs);

    bool isLostPkt() const;

    void setLostPkt(bool lostPkt);

    bool cmpJitter(const JitterData &a, const JitterData &b);

private:
    int seq;
    double jitter_ms;
    bool lostPkt;
};

class JitterQChartWidget : public QWidget
{
Q_OBJECT

private:
    Ui::JitterQChartWidget *ui;

    ControlWindow *ctrlWindow;

    QThread *thread;

    //int maxSize; // txData stores at most maxSize elements
    int maxX;
    int maxY;
    int msSinceLastUpdate;
    QList<JitterData> txData;
    QList<JitterData> rxData;

    //QList<int> txPktLoss;
    //QList<int> rxPktLoss;

    QChart *txChart;
    QChart *rxChart;
    QChartView *txChartView;
    QChartView *rxChartView;

    QLineSeries *txLineSeries;
    QLineSeries *rxLineSeries;
    QScatterSeries *txPacketLoss;
    QScatterSeries *rxPacketLoss;
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
    void receivePktLoss(int, unsigned int, unsigned int);
    void updatePktStatus(double mbs, double ps);

    void startTest(struct TestRunConfig *tConfig, struct ListenConfig *listenConfig);
    void stopTest();

};
