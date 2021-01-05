//
// Created by Pal-Erik Martinsen on 04/01/2021.
//

#include "chart.h"
#include <QDateTime>
#include <QHBoxLayout>
RealTimeCurveQChartWidget::RealTimeCurveQChartWidget(QWidget *parent) : QWidget(parent) {
    maxSize = 101; // Only store the latest 31 data
    maxX = 300;
    maxY = 100;
    numPkts = 0;
    splineSeries = new QSplineSeries();
    scatterSeries = new QScatterSeries();
    scatterSeries->setMarkerSize(8);
    chart = new QChart();
    chart->addSeries(splineSeries);
    chart->addSeries(scatterSeries);
    chart->legend()->hide();
    chart->setTitle("Real-time dynamic curve");
    chart->createDefaultAxes();
    chart->axisX()->setRange(0, 300);
    chart->axisY()->setRange(0, maxY);
    chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    QHBoxLayout *layout = new QHBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(chartView);
    setLayout(layout);
    timerId = startTimer(100);
    qsrand(QDateTime::currentDateTime().toTime_t());
}
RealTimeCurveQChartWidget::~RealTimeCurveQChartWidget() {
}
void RealTimeCurveQChartWidget::timerEvent(QTimerEvent *event) {
// Generate a data to simulate the continuous reception of new data
    if (event->timerId() == timerId) {
        int newData = qrand() % (maxY + 1);
        dataReceived(newData);
    }
}
void RealTimeCurveQChartWidget::dataReceived(int value) {
    data << value;
    //numPkts++;
// The number of data exceeds the maximum number, delete the first received data, and move the curve forward
    //while (data.size() > maxSize) {
    //    data.removeFirst();
    //}
// There is no need to draw data curves after the interface is hidden
    if (isVisible()) {
        splineSeries->clear();
        scatterSeries->clear();

        int dx = maxX / (maxSize-1);
        int less = maxSize - data.size();
        int start = data.size() - maxSize;
        if(start < 0){
            start = 0;
        }
        chart->axisX()->setRange(start,data.size());
        for (int i = start; i < data.size(); ++i) {
            //splineSeries->append((less*dx+i*dx)+numPkts, data.at(i));
            //scatterSeries->append((less*dx+i*dx)+numPkts, data.at(i));
            splineSeries->append(numPkts+i, data.at(i));
            scatterSeries->append(numPkts+i, data.at(i));
        }

    }

}