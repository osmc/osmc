#-------------------------------------------------
#
# Project created by QtCreator 2014-08-25T01:09:54
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = qt_target_installer
TEMPLATE = app

SOURCES += main.cpp\
        mainwindow.cpp \
    utils.cpp

HEADERS  += mainwindow.h \
    utils.h

FORMS    += mainwindow.ui

RESOURCES += \
    assets.qrc
