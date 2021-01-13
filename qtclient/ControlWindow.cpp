//
// Created by Pal-Erik Martinsen on 11/01/2021.
//

#include <QGridLayout>
#include <string>
#include <iostream>

#include "ui_JitterQChartWidget.h"

#include "iphelper.h"
#include "packettest.h"
#include "sockethelper.h"
#include "udptestcommon.h"
#include "ControlWindow.h"



ControlWindow::ControlWindow(QWidget *parent, Ui::JitterQChartWidget *ui,
                             struct TestRunConfig *tConfig, struct ListenConfig *listenConfig)
        : QDialog(parent)
{
    this->ui = ui;
    this->tConfig = tConfig;
    this->listenConfig = listenConfig;

    ui->interface->setText(this->listenConfig->interface);

    char              addrStr[SOCKADDR_MAX_STRLEN];
    sockaddr_toString( (struct sockaddr*)&listenConfig->localAddr,
                       addrStr,
                       sizeof(addrStr),
                       false );
    ui->source->setText(addrStr);
    ui->source->setReadOnly(true);


    sockaddr_toString( (struct sockaddr*)&listenConfig->remoteAddr,
                       addrStr,
                       sizeof(addrStr),
                       false );
    ui->destination->setText(addrStr);
    //connect(ui->destination, SIGNAL(textChanged(const QString &)), this, SLOT(changeDestination(const QString &)));
    connect(ui->destination, SIGNAL(editingFinished()), this, SLOT(changeDestination()));

    // connect(ui->destination, &QLineEdit::textChanged, this, SLOT(changeDestination(QString&)));

    int64_t delay = timespec_to_msec( &this->tConfig->pktConfig.delay );
    ui->delay->setRange(0,200);
    ui->delay->setValue(delay);
    //ui->delay->setText( QString::number(delay) );

    ui->pktSize->setRange(200, 1500);
    ui->pktSize->setValue(this->tConfig->pktConfig.pkt_size);
    connect(ui->pktSize, SIGNAL(valueChanged(int)), this, SLOT(changePktSize(int)));

    ui->burst->setRange(0,10);
    ui->burst->setValue( this->tConfig->pktConfig.pktsInBurst);


    ui->dscp->setText( "0x"+ QString::number( this->tConfig->pktConfig.dscp, 16));

    // Connect button signal to appropriate slot
    connect(ui->startBtn, &QPushButton::released, this, &ControlWindow::handleStartButton);
    connect(ui->stopBtn, &QPushButton::released, this, &ControlWindow::handleStopButton);
    connect(this, SIGNAL(startTest(struct TestRunConfig *, struct ListenConfig *)), parent, SLOT(startTest(struct TestRunConfig *, struct ListenConfig *)));
    connect(this, SIGNAL(stopTest()), parent, SLOT(stopTest()));
}


void ControlWindow::handleStartButton()
{
    emit startTest(tConfig, listenConfig);
}

void ControlWindow::handleStopButton()
{
    emit stopTest();
}

void ControlWindow::changePktSize(int value){
    emit stopTest();
    std::cout<<"New value:"<<value<<std::endl;
    tConfig->pktConfig.pkt_size = value;
    emit startTest(tConfig, listenConfig);

}

void ControlWindow::changeDestination(){
    QString s = ui->destination->text();
    std::cout<<"Changing detination: " << qUtf8Printable(s)<<std::endl;
    if ( !getRemoteIpAddr( (struct sockaddr*)&listenConfig->remoteAddr,
                           const_cast<char *>(s.toStdString().c_str()),
                           listenConfig->port ) ){
        printf("Error getting remote IPaddr");

    }
}