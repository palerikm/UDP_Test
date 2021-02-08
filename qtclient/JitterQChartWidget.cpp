
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
    //ui->rxJitter->setScaledContents(true);
    //ui->txJitter->setScaledContents(true);
    //ControlWindow
    //

    ctrlWindow = new ControlWindow(this, ui, tConfig, listenConfig);
    ui->gridLayout->addWidget(ctrlWindow, 0, 0);
    //ctrlWindow->show();
    //ctrlWindow->raise();
    //ctrlWindow->activateWindow();


    maxSize = 101; // Only store the latest 31 txData
    maxX = 100;
    maxY = 100;
    txLineSeries = new QSplineSeries();
    txLineSeries->setUseOpenGL(true);

    txPacketLoss = new QScatterSeries();
    txPacketLoss->setUseOpenGL(true);

    txChart = new QChart();
    txChart->addSeries(txLineSeries);
    txChart->addSeries(txPacketLoss);

    txChart->legend()->hide();
    txChart->setTitle("TX Jitter");
    txChart->createDefaultAxes();
    txChart->axes(Qt::Horizontal).first()->setRange(0 - maxX, 0);
    txChart->axes(Qt::Vertical).first()->setRange(-30, maxY);
    txChartView = new QChartView(txChart);
    txChartView->setRenderHint(QPainter::Antialiasing);
    ui->gridLayout->addWidget(txChartView, 1, 1);

    rxLineSeries = new QSplineSeries();
    rxLineSeries->setUseOpenGL(true);
    rxChart = new QChart();
    rxChart->addSeries(rxLineSeries);
    rxChart->legend()->hide();
    rxChart->setTitle("RX Jitter");
    rxChart->createDefaultAxes();
    rxChart->axes(Qt::Horizontal).first()->setRange(0 - maxX, 0);
    rxChart->axes(Qt::Vertical).first()->setRange(-30, maxY);
    rxChartView = new QChartView(rxChart);
    rxChartView->setRenderHint(QPainter::Antialiasing);
    ui->gridLayout->addWidget(rxChartView, 2, 1);

    thread = new QThread();
   // setup(tConfig, listenConfig);
}

JitterQChartWidget::~JitterQChartWidget()
{
    thread->quit();
    while(!thread->isFinished());

    delete thread;
    delete ui;
}

//void JitterQChartWidget::setup(struct TestRunConfig *tConfig, struct ListenConfig *listenConfig)
//{
//    thread = new QThread();
//    TestRunWorker *worker = new TestRunWorker(nullptr, tConfig, listenConfig);
//
//    connect(thread, SIGNAL(finished()), worker, SLOT(deleteLater()));
//    connect(this, SIGNAL(sendStartTestWorker()), worker, SLOT(startTests()));
//
//    connect(worker, SIGNAL(sendData(int, unsigned int, long)), this, SLOT(receiveData(int, unsigned int, long)));
//    connect(worker, SIGNAL(sendPktStatus(double, double)), this, SLOT(updatePktStatus(double, double)));
//
//
//    connect(thread, SIGNAL(finished()), worker, SLOT(deleteLater()));
//    worker->moveToThread(thread);
//
//
//    thread->start();
//
//    emit sendStartTestWorker();
//}

void JitterQChartWidget::startTest(struct TestRunConfig *tConfig, struct ListenConfig *listenConfig){
    txData.clear();
    rxData.clear();


    TestRunWorker *worker = new TestRunWorker(nullptr, tConfig, listenConfig);
    connect(thread, SIGNAL(finished()), worker, SLOT(deleteLater()));
    connect(this, SIGNAL(sendStartTestWorker()), worker, SLOT(startTests()));
    connect(this, SIGNAL(sendStopTestWorker()), worker, SLOT(stopTests()), Qt::DirectConnection);

    connect(worker, SIGNAL(sendData(int, unsigned int, long)), this, SLOT(receiveData(int, unsigned int, long)));
    connect(worker, SIGNAL(sendPktLoss(int, unsigned int, unsigned int)), this, SLOT(receivePktLoss(int, unsigned int, unsigned int)));
    connect(worker, SIGNAL(sendPktStatus(double, double)), this, SLOT(updatePktStatus(double, double)));


    worker->moveToThread(thread);

    thread->start();

    emit sendStartTestWorker();
}

void JitterQChartWidget::stopTest()
{
    std::cout<<"Trying to stop it.."<<std::endl;

    emit sendStopTestWorker();

    thread->quit();

    thread->wait();
    std::cout<<"Thread wait done"<<std::endl;
   // thread->terminate();

}

void JitterQChartWidget::updatePktStatus(double mbps, double ps){
    //std::cout<<"mbps: "<<mbps<<" ps: "<<ps<<std::endl;
    if (lcdNth % 20 != 0) {
        lcdNth++;
        return;
    }
    lcdNth = 1;
    ui->lcdMps->display(mbps);
    ui->lcdPs->display(ps);
}
void JitterQChartWidget::receivePktLoss(int id, unsigned int start, unsigned int stop){
    std::cout<<"Chart Pkt loss "<<id<<"  "<<start << "->"<<stop<<std::endl;
    if( id == 1){
        for(int i=0;i<=stop-start;i++){
            std::cout<<"Adding tx loss "<<start+i<<std::endl;
            txPktLoss.append(start+i);
        }
    }
    if( id == 2){
        for(int i=start;i<=stop;i++){
            std::cout<<"Adding rx loss "<<start+i<<std::endl;
            rxPktLoss.append(start+i);
        }
    }

}

void JitterQChartWidget::receiveData(int id, unsigned int seq, long jitter) {

    if (seq < 2) {
        return;
    }

    if(txPktLoss.count() > 0){
        double max = *std::max_element(txPktLoss.begin(), txPktLoss.end());
        int minSeq = seq-maxSize;
        if(max<minSeq){
            std::cout<<"Clearing pkt loss (no longer visible)"<<std::endl;
            txPktLoss.clear();
        }

    }

    if (id == 1) {
        txData << (double) jitter / 1000000;
        while (txData.size() > maxSize) {
            txData.removeFirst();
        }

    }


    if (id == 2) {
        rxData << (double) jitter / 1000000;
        while (rxData.size() > maxSize) {
            rxData.removeFirst();
        }
        if (txData.size() < 2) {
            return;
        }
    }
    if (isVisible()) {
        if (nth % 25 != 0) {
            nth++;
            return;
        }

        nth = 1;
        txLineSeries->clear();
        rxLineSeries->clear();

        txPacketLoss->clear();
        //rxPacketLoss->clear();

        int dx = maxX / (maxSize - 1);
        int txLess = maxSize - txData.size();
        int rxLess = maxSize - txData.size();

       // double txMin = *std::min_element(txData.begin(), txData.end());
        double txMax = *std::max_element(txData.begin(), txData.end());
       // double rxMin = *std::min_element(rxData.begin(), rxData.end());
        double rxMax = *std::max_element(rxData.begin(), rxData.end());
        double maxY = txMax > rxMax ? txMax : rxMax;
       // double minY = txMin < rxMin ? txMin : rxMin;
        maxY *= 1.1;
        //minY *= 1.5;

        //if(maxY> 10){
        //    QApplication::beep();
        //}
        //txChart->axes(Qt::Vertical).first()->setRange(minY, maxY);
        txChart->axes(Qt::Vertical).first()->setRange(-maxY, maxY);

        for (int i = 0; i < txData.size(); ++i) {

            int currX = (txLess * dx + i * dx) - maxX;
            //Do we have pkt loss we need to show?
            for (int j = 0; i < txPktLoss.size(); ++i) {
                    std::cout<<"Should att  loss at: "<< seq-txPktLoss.at(j)<<std::endl;
                    std::cout<<"seq: "<<seq<<"  paketloss seq:"<<txPktLoss.at(j)<<std::endl;
                //    txPacketLoss->append( seq-txPktLoss.at(j), 0);
            }

            txLineSeries->append(currX, txData.at(i));

           // std::cout<<i<<std::endl;
            //for (int j = 0; i < txPktLoss.size(); ++i) {
            //    std::cout<<"Adding loss at: "<< seq-txPktLoss.at(j)<<std::endl;
            //    std::cout<<"seq: "<<seq<<"  paketloss seq:"<<txPktLoss.at(j)<<std::endl;
            //    txPacketLoss->append( seq-txPktLoss.at(j), 0);
            //}
            txPacketLoss->append( (txLess * dx + i * dx) - maxX, 0);
        }


        //rxChart->axes(Qt::Vertical).first()->setRange(minY, maxY);
        rxChart->axes(Qt::Vertical).first()->setRange(-maxY, maxY);

        for (int i = 0; i < rxData.size(); ++i) {
            int currX = (rxLess * dx + i * dx) - maxX;
            rxLineSeries->append( currX, rxData.at(i));
        }
    }
}