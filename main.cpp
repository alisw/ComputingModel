#include "mainwindow.h"
#include <QApplication>

#include "alice.h"
#include "qfonticon.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QFontIcon::addFont(":/fontawesome.ttf");

    MainWindow w;
    w.show();


    return a.exec();
}
