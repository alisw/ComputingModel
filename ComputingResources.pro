#-------------------------------------------------
#
# Project created by QtCreator 2016-11-28T15:19:42
#
#-------------------------------------------------

QT       += core gui widgets printsupport
QT       += charts
QT       += network

contains(TARGET, qml.*) {
    QT += qml quick
}

CONFIG(release, debug|release):DEFINES += QT_NO_DEBUG_OUTPUT

TARGET = ComputingResources
TEMPLATE = app

include("QFontIcon/QFontIcon.pri")

INCLUDEPATH += QFontIcon

ICON = images/logo-alice.icns

SOURCES += main.cpp\
        mainwindow.cpp \
    logger.cpp \
    resources.cpp \
    alice.cpp \
    fundingagency.cpp \
    consolewidget.cpp \
    mymdiarea.cpp \
    tier.cpp \
    naming.cpp \
    pltablemodel.cpp

HEADERS  += mainwindow.h \
    logger.h \
    resources.h \
    alice.h \
    fundingagency.h \
    consolewidget.h \
    mymdiarea.h \
    tier.h \
    naming.h \
    pltablemodel.h

RESOURCES += \
    images.qrc \
    data/data.qrc

DISTFILES +=
