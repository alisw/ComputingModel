#include <QDebug>
#include <QPainter>
#include "mymdiarea.h"

MyMdiArea::MyMdiArea(QWidget *parent) : QMdiArea(parent)
{

}

//===========================================================================
void MyMdiArea::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(viewport());
    QPixmap background(":/images/Grid_sites_monitoring_map_-_ALICE_Grid_Monitoring_with_MonALISA.png");

    painter.drawPixmap(0, 0, width(), height(), background);

}
