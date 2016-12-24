#ifndef MYMDIAREA_H
#define MYMDIAREA_H

#include <QMdiArea>
#include <QObject>

class MyMdiArea : public QMdiArea
{
public:
    MyMdiArea(QWidget* parent = 0);

protected:
    void paintEvent(QPaintEvent * event);

};

#endif // MYMDIAREA_H
