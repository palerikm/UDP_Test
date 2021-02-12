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

    QStringList dscp_names = { "none", "CS0", "CS1", "AF11", "AF12", "AF13", "CS2", "AF21", "AF22", "AF23"
, "CS3", "AF31", "AF32", "CS4", "AF41", "AF42", "AF43", "CS5", "EF", "CS6", "CS7"};

    explicit ControlWindow(QWidget *parent = nullptr,
                           Ui::JitterQChartWidget *ui = nullptr,
                           struct TestRunConfig *tConfig = nullptr,
                           struct ListenConfig *listenConfig = nullptr);
    int getDelay();
private slots:
    void handleStartButton();
    void handleStopButton();
    void changeInterface(const QString &);
    void changePktSize(int);
    void changeDelay(int);
    void changeDestination(const QString &);
    void changeBurst(int value);
    void changeDscp(const QString &s);
    void dynamicYAxisChange(int state);
private:
    Ui::JitterQChartWidget *ui;
    struct TestRunConfig *tConfig;
    struct ListenConfig *listenConfig;

signals:
    void startTest(struct TestRunConfig *tConfig, struct ListenConfig *listenConfig);
    void stopTest();
};


#endif //UDP_TESTS_CONTROLWINDOW_H
