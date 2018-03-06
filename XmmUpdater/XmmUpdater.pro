#-------------------------------------------------
#
# Project created by QtCreator 2016-04-25T09:53:13
#
#-------------------------------------------------

QT       += core gui
QT       += network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Updater
TEMPLATE = app

RC_FILE = res.rc
DISTFILES += \
    res.rc


SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h

FORMS    += mainwindow.ui

RESOURCES += \
    resources.qrc



win32: LIBS += -L$$PWD/../build-quazip-Desktop_Qt_5_10_0_MinGW_32bit/quazip/release/ -lquazip

INCLUDEPATH += $$PWD/../quazip-0.7.2/quazip
DEPENDPATH += $$PWD/../quazip-0.7.2/quazip
