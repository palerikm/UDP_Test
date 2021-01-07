
#include <QTimer>
#include <QtGui>
#include <QDateTime>
#include <QHBoxLayout>
#include <iostream>

#include "JitterQChartWidget.h"
#include "ui_JitterQChartWidget.h"
#include "TestRunWorker.h"

JitterQChartWidget::JitterQChartWidget(QWidget *parent, struct TestRunConfig *tConfig, struct ListenConfig *listenConfig) :
        QWidget(parent),
        ui(new Ui::JitterQChartWidget)
{
    ui->setupUi(this);
    //ui->labelView->setScaledContents(true);
    ui->videoLabel->setScaledContents(true);
    ui->processedLabel_1->setScaledContents(true);


    maxSize = 101; // Only store the latest 31 data
    maxX = 300;
    maxY = 100;
    splineSeries = new QSplineSeries();
    scatterSeries = new QScatterSeries();
    scatterSeries->setMarkerSize(8);
    chart = new QChart();
    chart->addSeries(splineSeries);
    chart->addSeries(scatterSeries);
    chart->legend()->hide();
    chart->setTitle("Real-time dynamic curve");
    chart->createDefaultAxes();
    chart->axes(Qt::Horizontal).first()->setRange(0-maxX, 0);
    chart->axes(Qt::Vertical).first()->setRange(-30, maxY);
    chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    ui->gridLayout->addWidget(chartView, 1,0);

    setup(tConfig, listenConfig);
}

JitterQChartWidget::~JitterQChartWidget()
{
    thread->quit();
    while(!thread->isFinished());

    delete thread;
    delete ui;
}

void JitterQChartWidget::setup(struct TestRunConfig *tConfig, struct ListenConfig *listenConfig)
{
    thread = new QThread();
    TestRunWorker *worker = new TestRunWorker(nullptr, tConfig, listenConfig);

    connect(thread, SIGNAL(finished()), worker, SLOT(deleteLater()));
    connect(this, SIGNAL(sendStartTests()), worker, SLOT(startTests()));

    connect(worker, SIGNAL(sendData(int, unsigned int, long)), this, SLOT(receiveData(int, unsigned int, long)));


    connect(thread, SIGNAL(finished()), worker, SLOT(deleteLater()));

    worker->moveToThread(thread);


    thread->start();

    emit sendStartTests();
}
void JitterQChartWidget::receiveData(int id, unsigned int seq, long jitter) {

    if(id != 1){
        return;
    }
    if(seq < 2){
        return;
    }
    std::cout<<"data: " << jitter<<std::endl;
    data << (double)jitter/1000000;

// The number of data exceeds the maximum number, delete the first received data, and move the curve forward
    while (data.size() > maxSize) {
        data.removeFirst();
    }
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
        //chart->axisX()->setRange(start,);
        //

        double min = *std::min_element(data.begin(), data.end());
        double max = *std::max_element(data.begin(), data.end());
        chart->axes(Qt::Vertical).first()->setRange(min-1, max+1);
        for (int i = start; i < data.size(); ++i) {
            splineSeries->append((less*dx+i*dx)-maxX, data.at(i));
            //scatterSeries->append((less*dx+i*dx)-maxX, data.at(i));
            //splineSeries->append(numPkts+i, data.at(i));
            //scatterSeries->append(numPkts+i, data.at(i));
        }

    }
}

#if 0
JitterQChartWidget::JitterQChartWidget(QWidget *parent) : QWidget(parent), ui(new Ui::JitterQChartWidget){
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


JitterQChartWidget::~JitterQChartWidget() {
}
void JitterQChartWidget::timerEvent(QTimerEvent *event) {
// Generate a data to simulate the continuous reception of new data
    if (event->timerId() == timerId) {
        int newData = qrand() % (maxY + 1);
        dataReceived(newData);
    }
}
void JitterQChartWidget::dataReceived(int value) {
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
#endif