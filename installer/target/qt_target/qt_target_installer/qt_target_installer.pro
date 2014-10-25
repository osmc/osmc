#-------------------------------------------------
#
# Project created by QtCreator 2014-08-25T01:09:54
#
#-------------------------------------------------

QT       += core gui

CONFIG += console

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = qt_target_installer
TEMPLATE = app

SOURCES += main.cpp\
        mainwindow.cpp \
    utils.cpp \
    logger.cpp \
    cmdlineparser.cpp \
    extractworker.cpp \
    preseedparser.cpp \
    target.cpp \
    targetlist.cpp \
    network.cpp \
    bootloaderconfig.cpp

HEADERS  += mainwindow.h \
    utils.h \
    logger.h \
    cmdlineparser.h \
    extractworker.h \
    preseedparser.h \
    target.h \
    targetlist.h \
    network.h \
    bootloaderconfig.h

FORMS    += mainwindow.ui

RESOURCES += \
    assets.qrc
