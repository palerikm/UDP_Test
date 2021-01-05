//
// Created by Pal-Erik Martinsen on 04/01/2021.
//

#ifndef UDP_TESTS_CHART_H
#define UDP_TESTS_CHART_H


#include <QWidget>
#include <QList>
#include <QSplineSeries>
#include <QScatterSeries>
#include <QChart>
#include <QChartView>
using namespace QtCharts;
class RealTimeCurveQChartWidget : public QWidget {
Q_OBJECT
public:
    explicit RealTimeCurveQChartWidget(QWidget *parent = 0);
    ~RealTimeCurveQChartWidget();
protected:
    void timerEvent(QTimerEvent *event) Q_DECL_OVERRIDE;
private:
/**
* Received the data sent from the data source, the data source can be lower computer, acquisition card, sensor, etc.
*/
    void dataReceived(int value);
    int timerId;
    int maxSize; // data stores at most maxSize elements
    int maxX;
    int maxY;
    int numPkts;
    QList<double> data; // A list to store business data
    QChart *chart;
    QChartView *chartView;
    QSplineSeries *splineSeries;
    QScatterSeries *scatterSeries;
};

#endif //UDP_TESTS_CHART_H
