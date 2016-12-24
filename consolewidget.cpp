// A console to be included in a mdi view
// Y. Schutz November 2016 (Thanks Sacha)

#include <QPlainTextEdit>
#include <QVBoxLayout>

#include "consolewidget.h"

//===========================================================================
ConsoleWidget::ConsoleWidget(QWidget *parent) : QWidget(parent)
{
    // ctor the Console is a QPlainTextEdit widget read only mode

    QVBoxLayout * vLayout = new QVBoxLayout();
    mTextEdit = new QPlainTextEdit();
//    mTextEdit->setReadOnly(true);


    vLayout->addWidget(mTextEdit);
    vLayout->setContentsMargins(0,0,0,0);

    setLayout(vLayout);
}

//===========================================================================
void ConsoleWidget::setMessage(const QString &message)
{
    // writes out a message to the console

    mTextEdit->appendPlainText(message);

}
