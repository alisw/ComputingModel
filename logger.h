// Author: Yves Schutz 23 novembre 2016
//
// Object to redefine logging. Singleton
#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>

class Logger : public QObject
{
    Q_OBJECT
public:
    static Logger* instance();
    static void    write(const QString& message);

Q_SIGNALS:
    void messageReceived(const QString& message);

private:
   Logger(QObject *parent = 0);
   ~Logger() {}
   Logger (const Logger&) {}

   void writeMessage(const QString& message);

    static Logger* mInstance; // the unique instance
    QString        mMessage;  // the message to be displayed
};

#endif // LOGGER_H
