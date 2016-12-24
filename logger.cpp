// Author: Yves Schutz 23 novembre 2016
//
// Object to redefine logging. Singleton

#include "logger.h"

Logger *Logger::mInstance = Q_NULLPTR;

//__________________________________________________________________________
Logger::Logger(QObject *parent) :QObject(parent)
{
    // ctor (private)
    setObjectName("Logger");
}

//__________________________________________________________________________
void Logger::writeMessage(const QString &message)
{
    // tells that message is available for writing

    mMessage = message;
    emit messageReceived(message);
}

//__________________________________________________________________________
Logger *Logger::instance()
{
    // returns the unique instance
    if (!mInstance)
        mInstance = new Logger();

    return mInstance;
}

//__________________________________________________________________________
void Logger::write(const QString &message)
{
    // writes the message

    Logger::instance()->writeMessage(message);
}
