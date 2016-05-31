#-------------------------------------------------
#
# Project created by QtCreator 2016-03-30T15:12:07
#
#-------------------------------------------------

QT       += core gui
QT       += network
QT       += serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = VoltageMonitor
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    tcpserver.cpp \
    serialcommunication.cpp \
    utils.cpp \
    server.cpp

HEADERS  += mainwindow.h \
    tcpserver.h \
    serialcommunication.h \
    utils.h \
    server.h

FORMS    += mainwindow.ui

RESOURCES += \
    images.qrc
