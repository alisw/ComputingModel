#-------------------------------------------------
#
# Project created by QtCreator 2016-11-28T15:19:42
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ComputingResources
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    logger.cpp \
    resources.cpp \
    alice.cpp \
    fundingagency.cpp \
    consolewidget.cpp \
    mymdiarea.cpp \
    tier.cpp \
    naming.cpp

HEADERS  += mainwindow.h \
    logger.h \
    resources.h \
    alice.h \
    fundingagency.h \
    consolewidget.h \
    mymdiarea.h \
    tier.h \
    naming.h

RESOURCES += \
    images.qrc \
    data/data.qrc

DISTFILES +=
