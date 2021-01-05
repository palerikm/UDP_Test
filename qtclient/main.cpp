//
// Created by Pal-Erik Martinsen on 04/01/2021.
//


#include "chart.h"
#include <QApplication>
int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    RealTimeCurveQChartWidget w;
    w.resize(700, 400);
    w.show();
    return a.exec();
}


