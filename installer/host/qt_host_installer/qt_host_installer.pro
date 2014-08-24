#-------------------------------------------------
#
# Project created by QtCreator 2014-07-29T22:28:04
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets network

TARGET = qt_host_installer
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    utils.cpp \
    io.cpp \
    langselection.cpp \
    updatenotification.cpp \
    supporteddevice.cpp \
    versionselection.cpp

HEADERS  += mainwindow.h \
    utils.h \
    io.h \
    langselection.h \
    updatenotification.h \
    supporteddevice.h \
    versionselection.h

FORMS    += mainwindow.ui \
    langselection.ui \
    updatenotification.ui \
    versionselection.ui

VERSION = 1.0.0

TRANSLATIONS = osmc_da.ts \
    osmc_de.ts \
    osmc_es.ts \
    osmc_fr.ts \
    osmc_nl.ts \
    osmc_ru.ts \

RESOURCES += \
    assets.qrc
