#pragma once


#include <QWidget>
#include <QList>
#include <QSplineSeries>
#include <QScatterSeries>
#include <QChart>
#include <QChartView>

using namespace QtCharts;

namespace Ui {
    class JitterQChartWidget;
}

class JitterQChartWidget : public QWidget
{
Q_OBJECT

private:
    Ui::JitterQChartWidget *ui;
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

    void setup(struct TestRunConfig *tConfig, struct ListenConfig *listenConfig);

public:
    explicit JitterQChartWidget(QWidget *parent = 0, struct TestRunConfig *tConfig = nullptr, struct ListenConfig *listenConfig = nullptr);

    ~JitterQChartWidget();

protected:
    //  void resizeEvent(QResizeEvent* event) override;



signals:

    void sendStartTests();


private slots:
    void receiveData(int, unsigned int, long);
    //void receiveFrame(QImage frame);

    //void receiveProcessedFrame(QImage frame1, QImage frame2, QImage frame3);

    //void receiveActualFps(int actualFps);

    //oid receiveReadFrameTime(int time);

    //void receiveProcessTime(int time);
};

#if 0
using namespace QtCharts;

class JitterQChartWidget : public QWidget {
    Q_OBJECT

public:
    explicit JitterQChartWidget(QWidget *parent = 0);
    ~JitterQChartWidget();

protected:
    void timerEvent(QTimerEvent *event) Q_DECL_OVERRIDE;

private:
    Ui::JitterQChartWidget *ui;
    QThread *thread;


    void dataReceived(int value);
    int timerId;
    int maxSize; // txData stores at most maxSize elements
    int maxX;
    int maxY;
    int numPkts;
    QList<double> txData; // A list to store business txData
    QChart *txChart;
    QChartView *txChartView;
    QSplineSeries *txLineSeries;
    QScatterSeries *scatterSeries;
};

#endif