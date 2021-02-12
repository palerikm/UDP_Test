//
// Created by Pal-Erik Martinsen on 11/01/2021.
//

#include <QGridLayout>
#include <string>
#include <iostream>

#include <sockethelper.h>
#include <iphelper.h>
#include <timing.h>
#include <QMenu>
#include "ui_JitterQChartWidget.h"


#include "ControlWindow.h"



ControlWindow::ControlWindow(QWidget *parent, Ui::JitterQChartWidget *ui,
                             struct TestRunConfig *tConfig, struct ListenConfig *listenConfig)
        : QDialog(parent)
{
    this->ui = ui;
    this->tConfig = tConfig;
    this->listenConfig = listenConfig;

    ui->stopBtn->setDisabled((true));

    ui->interface->setText(this->listenConfig->interface);
    connect(ui->interface, SIGNAL(textChanged(const QString &)), this, SLOT(changeInterface(const QString &)));

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
    connect(ui->destination, SIGNAL(textChanged(const QString &)), this, SLOT(changeDestination(const QString &)));


    int64_t delay = timespec_to_msec( &this->tConfig->pktConfig.delay );
    ui->delay->setRange(0,200);
    ui->delay->setValue(delay);
    connect(ui->delay, SIGNAL(valueChanged(int)), this, SLOT(changeDelay(int)));

    ui->pktSize->setRange(200, 1500);
    ui->pktSize->setValue(this->tConfig->pktConfig.pkt_size);
    connect(ui->pktSize, SIGNAL(valueChanged(int)), this, SLOT(changePktSize(int)));

    ui->burst->setRange(0,10);
    ui->burst->setValue( this->tConfig->pktConfig.pktsInBurst);
    connect(ui->burst, SIGNAL(valueChanged(int)), this, SLOT(changeBurst(int)));


    ui->dscp_combo->addItems(dscp_names);

    connect(ui->dscp_combo, SIGNAL(currentTextChanged(const QString &)), this, SLOT(changeDscp(const QString &)));
    // Connect button signal to appropriate slot
    connect(ui->startBtn, &QPushButton::released, this, &ControlWindow::handleStartButton);
    connect(ui->stopBtn, &QPushButton::released, this, &ControlWindow::handleStopButton);
    connect(this, SIGNAL(startTest(struct TestRunConfig *, struct ListenConfig *)), parent, SLOT(startTest(struct TestRunConfig *, struct ListenConfig *)));
    connect(this, SIGNAL(stopTest()), parent, SLOT(stopTest()));
}

int ControlWindow::getDelay(){
    return (int)timespec_to_msec( &tConfig->pktConfig.delay);

}
void ControlWindow::handleStartButton()
{
    ui->startBtn->setDisabled((true));
    ui->stopBtn->setDisabled((false));
    emit startTest(tConfig, listenConfig);
}

void ControlWindow::handleStopButton()
{
    ui->startBtn->setDisabled((false));
    ui->stopBtn->setDisabled((true));
    emit stopTest();
}

void ControlWindow::changePktSize(int value){
    emit stopTest();
    tConfig->pktConfig.pkt_size = value;
}

void ControlWindow::changeDelay(int value){
    emit stopTest();
    tConfig->pktConfig.delay =  timespec_from_ms(value);
}

void ControlWindow::changeBurst(int value){
    emit stopTest();
    tConfig->pktConfig.pktsInBurst = value;
}

void ControlWindow::changeDscp(const QString &s){
    emit stopTest();
    int tos = 0;
    if(s == "none"){
       tos = 0;
    }else if(s == "CS0"){
        tos = 0;
    }else if(s == "CS1"){
        tos = 32;
    }
    else if(s == "AF11"){
        tos = 40;
    }else if(s == "AF12"){
        tos = 48;
    }else if(s == "AF13"){
        tos = 56;
    }else if(s == "CS2"){
        tos = 64;
    }else if(s == "AF21"){
        tos = 72;
    }else if(s == "AF22"){
        tos = 80;
    }else if(s == "AF23"){
        tos = 88;
    }else if(s == "CS3"){
        tos = 96;
    }else if(s == "AF31"){
        tos = 104;
    }else if(s == "AF32"){
        tos = 112;
    }else if(s == "AF33"){
        tos = 120;
    }else if(s == "CS4"){
        tos = 128;
    }else if(s == "AF41"){
        tos = 136;
    }else if(s == "AF42"){
        tos = 144;
    }else if(s == "AF43"){
        tos = 152;
    }else if(s == "CS5"){
        tos = 160;
    }else if(s == "EF"){
        tos = 184;
    }else if(s == "CS6"){
        tos = 192;
    }else if(s == "CS7"){
        tos = 224;
    }

    tConfig->pktConfig.tos = tos;
}

void ControlWindow::changeDestination(const QString &s){
    emit stopTest();
    //QString s = ui->destination->text();
    if ( !getRemoteIpAddr( (struct sockaddr*)&listenConfig->remoteAddr,
                           const_cast<char *>(s.toStdString().c_str()),
                           listenConfig->port ) ){
        printf("Error getting remote IPaddr");
    }
}

void ControlWindow::changeInterface(const QString &s){
    emit stopTest();

    strncpy(listenConfig->interface, s.toStdString().c_str(), sizeof(listenConfig->interface));
    if ( !getLocalInterFaceAddrs( (struct sockaddr*)&listenConfig->localAddr,
                                  listenConfig->interface,
                                  listenConfig->remoteAddr.ss_family,
                                  IPv6_ADDR_NORMAL,
                                  false ) )
    {
        printf("Error getting IPaddr on %s\n", listenConfig->interface);
        ui->interface->setStyleSheet("color: red;");
        ui->source->setText("N/A");
    }else{
        ui->interface->setStyleSheet("color: white;");
        char              addrStr[SOCKADDR_MAX_STRLEN];
        sockaddr_toString( (struct sockaddr*)&listenConfig->localAddr,
                           addrStr,
                           sizeof(addrStr),
                           false );
        ui->source->setText(addrStr);
        ui->source->setReadOnly(true);
    }
}