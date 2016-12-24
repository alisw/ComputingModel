// A console to be included in a mdi view
// Y. Schutz November 2016 (Thanks Sacha)

#ifndef CONSOLEWIDGET_H
#define CONSOLEWIDGET_H

#include <QWidget>

class QPlainTextEdit;

class ConsoleWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ConsoleWidget(QWidget *parent = 0);

public Q_SLOTS:
    void setMessage(const QString& message);


private:
    QPlainTextEdit * mTextEdit;
};

#endif // CONSOLEWIDGET_H
