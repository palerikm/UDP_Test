//
// Created by Pal-Erik Martinsen on 11/01/2021.
//

#ifndef UDP_TESTS_CONTROLWINDOW_H
#define UDP_TESTS_CONTROLWINDOW_H

#include <QWidget>
#include <QDialog>
#include <QPushButton>

#include "udpjitter.h"

namespace Ui {
    class JitterQChartWidget;
}

class ControlWindow : public QDialog{
Q_OBJECT
public:
    explicit ControlWindow(QWidget *parent = nullptr,
                           Ui::JitterQChartWidget *ui = nullptr,
                           struct TestRunConfig *tConfig = nullptr,
                           struct ListenConfig *listenConfig = nullptr);
    int getDelay();
private slots:
    void handleStartButton();
    void handleStopButton();
    void changeInterface();
    void changePktSize(int);
    void changeDelay(int);
    void changeDestination();
    void changeBurst(int value);
    void changeDscp();
private:
    Ui::JitterQChartWidget *ui;
    struct TestRunConfig *tConfig;
    struct ListenConfig *listenConfig;

signals:
    void startTest(struct TestRunConfig *tConfig, struct ListenConfig *listenConfig);
    void stopTest();
};


#endif //UDP_TESTS_CONTROLWINDOW_H
