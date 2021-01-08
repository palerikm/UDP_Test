
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


    maxSize = 101; // Only store the latest 31 txData
    maxX = 300;
    maxY = 100;
    txLineSeries = new QSplineSeries();
    txLineSeries->setUseOpenGL(true);

//    scatterSeries = new QScatterSeries();
//    scatterSeries->setMarkerSize(8);
    txChart = new QChart();
    txChart->addSeries(txLineSeries);
//    txChart->addSeries(scatterSeries);
    txChart->legend()->hide();
    txChart->setTitle("TX Jitter");
    txChart->createDefaultAxes();
    txChart->axes(Qt::Horizontal).first()->setRange(0 - maxX, 0);
    txChart->axes(Qt::Vertical).first()->setRange(-30, maxY);
    txChartView = new QChartView(txChart);
    txChartView->setRenderHint(QPainter::Antialiasing);
    ui->gridLayout->addWidget(txChartView, 1, 0);

    rxLineSeries = new QSplineSeries();
    rxLineSeries->setUseOpenGL(true);
    rxChart = new QChart();
    rxChart->addSeries(rxLineSeries);
//    txChart->addSeries(scatterSeries);
    rxChart->legend()->hide();
    rxChart->setTitle("RX Jitter");
    rxChart->createDefaultAxes();
    rxChart->axes(Qt::Horizontal).first()->setRange(0 - maxX, 0);
    rxChart->axes(Qt::Vertical).first()->setRange(-30, maxY);
    rxChartView = new QChartView(rxChart);
    rxChartView->setRenderHint(QPainter::Antialiasing);
    ui->gridLayout->addWidget(rxChartView, 2, 0);

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

    if(seq < 2){
        return;
    }
    if(id == 1){
        txData << (double)jitter / 1000000;
        while (txData.size() > maxSize) {
            txData.removeFirst();
        }

    }


    if(id == 2) {
        rxData << (double) jitter / 1000000;
        while (rxData.size() > maxSize) {
            rxData.removeFirst();
        }
        if (txData.size() < 2) {
            return;
        }
    }
    if (isVisible()) {
        if(nth%5 != 0){
            nth++;
            return;
        }
        nth=1;
        txLineSeries->clear();
        rxLineSeries->clear();

        int dx = maxX / (maxSize-1);
        int txLess = maxSize - txData.size();
        int rxLess = maxSize - txData.size();
        int txStart = txData.size() - maxSize;
        int rxStart = txData.size() - maxSize;

        //txChart->axisX()->setRange(start,);
        //

        double txMin = *std::min_element(txData.begin(), txData.end());
        double txMax = *std::max_element(txData.begin(), txData.end());
        double rxMin = *std::min_element(rxData.begin(), rxData.end());
        double rxMax = *std::max_element(rxData.begin(), rxData.end());
        double maxY = txMax > rxMax ? txMax : rxMax;
        double minY = txMin < rxMin ? txMin : rxMin;
        maxY*=1.5;
        minY*=1.5;

        txChart->axes(Qt::Vertical).first()->setRange(minY, maxY);
        for (int i = 0; i < txData.size(); ++i) {
            txLineSeries->append((txLess * dx + i * dx) - maxX, txData.at(i));
            //scatterSeries->append((less*dx+i*dx)-maxX, txData.at(i));
            //txLineSeries->append(numPkts+i, txData.at(i));
            //scatterSeries->append(numPkts+i, txData.at(i));
        }

        rxChart->axes(Qt::Vertical).first()->setRange(minY, maxY);
        for (int i = 0; i < rxData.size(); ++i) {
            rxLineSeries->append((rxLess * dx + i * dx) - maxX, rxData.at(i));
            //scatterSeries->append((less*dx+i*dx)-maxX, txData.at(i));
            //txLineSeries->append(numPkts+i, txData.at(i));
            //scatterSeries->append(numPkts+i, txData.at(i));
        }


    }



// The number of txData exceeds the maximum number, delete the first received txData, and move the curve forward

// There is no need to draw txData curves after the interface is hidden

}

#if 0
JitterQChartWidget::JitterQChartWidget(QWidget *parent) : QWidget(parent), ui(new Ui::JitterQChartWidget){
    maxSize = 101; // Only store the latest 31 txData
    maxX = 300;
    maxY = 100;
    numPkts = 0;
    txLineSeries = new QSplineSeries();
    scatterSeries = new QScatterSeries();
    scatterSeries->setMarkerSize(8);
    txChart = new QChart();
    txChart->addSeries(txLineSeries);
    txChart->addSeries(scatterSeries);
    txChart->legend()->hide();
    txChart->setTitle("Real-time dynamic curve");
    txChart->createDefaultAxes();
    txChart->axisX()->setRange(0, 300);
    txChart->axisY()->setRange(0, maxY);
    txChartView = new QChartView(txChart);
    txChartView->setRenderHint(QPainter::Antialiasing);
    QHBoxLayout *layout = new QHBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(txChartView);
    setLayout(layout);
    timerId = startTimer(100);
    qsrand(QDateTime::currentDateTime().toTime_t());
}


JitterQChartWidget::~JitterQChartWidget() {
}
void JitterQChartWidget::timerEvent(QTimerEvent *event) {
// Generate a txData to simulate the continuous reception of new txData
    if (event->timerId() == timerId) {
        int newData = qrand() % (maxY + 1);
        dataReceived(newData);
    }
}
void JitterQChartWidget::dataReceived(int value) {
    txData << value;
    //numPkts++;
// The number of txData exceeds the maximum number, delete the first received txData, and move the curve forward
    //while (txData.size() > maxSize) {
    //    txData.removeFirst();
    //}
// There is no need to draw txData curves after the interface is hidden
    if (isVisible()) {
        txLineSeries->clear();
        scatterSeries->clear();

        int dx = maxX / (maxSize-1);
        int less = maxSize - txData.size();
        int start = txData.size() - maxSize;
        if(start < 0){
            start = 0;
        }
        txChart->axisX()->setRange(start,txData.size());
        for (int i = start; i < txData.size(); ++i) {
            //txLineSeries->append((less*dx+i*dx)+numPkts, txData.at(i));
            //scatterSeries->append((less*dx+i*dx)+numPkts, txData.at(i));
            txLineSeries->append(numPkts+i, txData.at(i));
            scatterSeries->append(numPkts+i, txData.at(i));
        }

    }

}
#endif