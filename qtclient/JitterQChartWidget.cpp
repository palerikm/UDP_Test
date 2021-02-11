
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

    timer = new QTimer(this);

    ctrlWindow = new ControlWindow(this, ui, tConfig, listenConfig);
    ui->gridLayout->addWidget(ctrlWindow, 0, 0);


    maxX = 1000;
    maxY = 10;
    //txLineSeries = new QSplineSeries();
    txLineSeries = new QLineSeries();
    txLineSeries->setUseOpenGL(true);

    txPacketLoss = new QScatterSeries();
    txPacketLoss->setUseOpenGL(true);
    txPacketLoss->setColor(Qt::red);

    txChart = new QChart();
    txChart->setDropShadowEnabled(false);
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

    //rxLineSeries = new QSplineSeries();
    rxLineSeries = new QLineSeries();

    rxLineSeries->setUseOpenGL(true);

    rxPacketLoss = new QScatterSeries();
    rxPacketLoss->setUseOpenGL(true);
    rxPacketLoss->setColor(Qt::red);

    rxChart = new QChart();
    rxChart->setDropShadowEnabled(false);
    rxChart->addSeries(rxLineSeries);
    rxChart->addSeries(rxPacketLoss);
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



void JitterQChartWidget::startTest(struct TestRunConfig *tConfig, struct ListenConfig *listenConfig){
    txData.clear();
    rxData.clear();


    TestRunWorker *worker = new TestRunWorker(nullptr, tConfig, listenConfig);
    connect(thread, SIGNAL(finished()), worker, SLOT(deleteLater()));
    connect(thread, SIGNAL(finished()), this, SLOT(workerDone()));
    connect(this, SIGNAL(sendStartTestWorker()), worker, SLOT(startTests()));
    connect(this, SIGNAL(sendStopTestWorker()), worker, SLOT(stopTests()), Qt::DirectConnection);

    connect(worker, SIGNAL(sendData(int, unsigned int, long)), this, SLOT(receiveData(int, unsigned int, long)));
    connect(worker, SIGNAL(sendPktLoss(int, unsigned int, unsigned int)), this, SLOT(receivePktLoss(int, unsigned int, unsigned int)));
    connect(worker, SIGNAL(sendPktStatus(double, double)), this, SLOT(updatePktStatus(double, double)));


    worker->moveToThread(thread);

    thread->start();


    connect(timer, SIGNAL(timeout()), this, SLOT(updateCharts()));
    timer->start(50);

    emit sendStartTestWorker();
}

void JitterQChartWidget::stopTest()
{
    std::cout<<"Trying to stop it.."<<std::endl;
    timer->stop();
    emit sendStopTestWorker();

    thread->quit();

    thread->wait();
    std::cout<<"Thread wait done"<<std::endl;
   // thread->terminate();

}

void JitterQChartWidget::workerDone() {
    std::cout<<"Worker is done.. Byt why? Did I do it or did the server crash?"<<std::endl;
}

void JitterQChartWidget::updatePktStatus(double mbps, double ps){
    //std::cout<<"mbps: "<<mbps<<" ps: "<<ps<<std::endl;
    if (lcdNth % 100 != 0) {
        lcdNth++;
        return;
    }
    lcdNth = 1;
    ui->lcdMps->display(mbps);
    ui->lcdPs->display(ps);
}
void JitterQChartWidget::receivePktLoss(int id, unsigned int start, unsigned int stop){
    QApplication::beep();
    if( id == 1){
        for(int i=0;i<=stop-start;i++){
            JitterData d(id, start+1, 0, true);
            txData.append(d);
        }
    }
    if( id == 2){
        for(int i=start;i<=stop;i++){
            JitterData d(id, start+1, 0, true);
            rxData.append(d);
        }
    }
    while (txData.size() > maxX) {
        txData.removeFirst();
    }
    while (rxData.size() > maxX) {
        rxData.removeFirst();
    }
}

void JitterQChartWidget::receiveData(int id, unsigned int seq, long jitter) {

    if (seq < 2) {
        return;
    }

    if (id == 1) {
        JitterData d(id, seq, jitter / 1000000, false);
        txData << d;
        while (txData.size() > maxX) {
            txData.removeFirst();
        }
    }

    if (id == 2) {
        JitterData d(id, seq, jitter / 1000000, false);
        rxData << d;
        while (rxData.size() > maxX) {
            rxData.removeFirst();
        }
    }

    if( !txData.isEmpty()) {
        int seqDiff = abs(rxData.last().getSeq() - txData.last().getSeq());
        if( seqDiff > 100)
            std::cout << "SeqDiff :" << seqDiff << std::endl;
    }

}


void JitterQChartWidget::updateCharts(){
    if (txData.size() < 2) {
        return;
    }
    if (isVisible()) {
        txLineSeries->clear();
        rxLineSeries->clear();

        txPacketLoss->clear();
        rxPacketLoss->clear();

        double txMax = std::max_element(txData.begin(), txData.end(),
                                        [] (JitterData const& lhs, JitterData const& rhs) {return lhs.getJitterMs() < rhs.getJitterMs();})->getJitterMs();


        double rxMax = std::max_element(rxData.begin(), rxData.end(),
                                        [] (JitterData const& lhs, JitterData const& rhs) {return lhs.getJitterMs() < rhs.getJitterMs();})->getJitterMs();


        maxY = txMax > rxMax ? txMax : rxMax;

       // maxY *= 1.1;

        txChart->axes(Qt::Vertical).first()->setRange(-maxY, maxY);
        

        int currX= std::min(maxX,  txData.size()) * -1;
        // std::cout<<"Updating TX graph!"<<std::endl;
        for (int i = 0; i < txData.size(); ++i) {

            // int currX = (txLess * dx + i * dx) - maxX;
            currX ++;
            // std::cout<<"Tx currX : "<<currX<<" i: "<< i<<"size(): "<<txData.size()<<std::endl;
            //Do we have pkt loss we need to show?
            if(txData.at(i).isLostPkt()){
                txPacketLoss->append(currX, 0);
            }else{
                txLineSeries->append(currX, txData.at(i).getJitterMs());
            }
        }
        // std::cout<<"Updating TX graph  (stop)!"<<std::endl;

        //rxChart->axes(Qt::Vertical).first()->setRange(minY, maxY);
        rxChart->axes(Qt::Vertical).first()->setRange(-maxY, maxY);

        currX= std::min(maxX,  rxData.size()) * -1;
        for (int i = 0; i < rxData.size(); ++i) {
            //int currX = (rxLess * dx + i * dx) - maxX;
            currX ++;
            if(rxData.at(i).isLostPkt()){
                rxPacketLoss->append(currX, 0);
            }else{
                rxLineSeries->append(currX, rxData.at(i).getJitterMs());
            }
        }
    }
}
