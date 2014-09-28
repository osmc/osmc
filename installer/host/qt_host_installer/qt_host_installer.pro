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
    versionselection.cpp \
    preseeddevice.cpp \
    licenseagreement.cpp \
    networksetup.cpp \
    deviceselection.cpp \
    networksettings.cpp \
    advancednetworksetup.cpp \
    wifinetworksetup.cpp \
    nixdiskdevice.cpp \
    downloadprogress.cpp \
    extractprogress.cpp \
    successdialog.cpp \
    preseeder.cpp \
    extractworker.cpp \
    writeimageworker.cpp

macx {
SOURCES += \
    io_osx.cpp
}

unix:!macx {
SOURCES += \
    io_linux.cpp
}

win32 {
SOURCES += \
    io_windows.cpp
}
LIBS += -lz

win32 {
    INCLUDEPATH += "w32-lib/zlib-1.2.8"
    LIBS += -L./w32-lib/zlib-1.2.8
}

HEADERS  += mainwindow.h \
    utils.h \
    langselection.h \
    updatenotification.h \
    supporteddevice.h \
    versionselection.h \
    preseeddevice.h \
    licenseagreement.h \
    networksetup.h \
    deviceselection.h \
    networksettings.h \
    advancednetworksetup.h \
    wifinetworksetup.h \
    nixdiskdevice.h \
    downloadprogress.h \
    extractprogress.h \
    successdialog.h \
    preseeder.h \
    extractworker.h \
    writeimageworker.h \
    io.h

FORMS    += mainwindow.ui \
    langselection.ui \
    updatenotification.ui \
    versionselection.ui \
    preseeddevice.ui \
    licenseagreement.ui \
    networksetup.ui \
    deviceselection.ui \
    advancednetworksetup.ui \
    wifinetworksetup.ui \
    downloadprogress.ui \
    extractprogress.ui \
    successdialog.ui

VERSION = 1.0.1

TRANSLATIONS = osmc_da.ts \
    osmc_de.ts \
    osmc_es.ts \
    osmc_fr.ts \
    osmc_nl.ts \
    osmc_ru.ts \

RESOURCES += \
    assets.qrc

CONFIG += static
