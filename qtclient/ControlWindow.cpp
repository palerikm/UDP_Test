//
// Created by Pal-Erik Martinsen on 11/01/2021.
//

#include <QGridLayout>

#include "ui_JitterQChartWidget.h"

#include "ControlWindow.h"



ControlWindow::ControlWindow(QWidget *parent, Ui::JitterQChartWidget *ui,
                             struct TestRunConfig *tConfig, struct ListenConfig *listenConfig)
        : QDialog(parent)
{
    this->ui = ui;
    this->tConfig = tConfig;
    this->listenConfig = listenConfig;
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