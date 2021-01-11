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

    int maxSize; // txData stores at most maxSize elements
    int maxX;
    int maxY;
    int nth;
    QList<double> txData;
    QList<double> rxData;
    QChart *txChart;
    QChart *rxChart;
    QChartView *txChartView;
    QChartView *rxChartView;

    QLineSeries *txLineSeries;
    QLineSeries *rxLineSeries;



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

};
