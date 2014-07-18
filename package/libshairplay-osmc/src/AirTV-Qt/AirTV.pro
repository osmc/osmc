#-------------------------------------------------
#
# Project created by QtCreator 2011-12-04T00:47:51
#
#-------------------------------------------------

QT       += core gui multimedia phonon svg

include(qtsingleapplication/src/qtsingleapplication.pri)

TARGET = AirTV
TEMPLATE = app
ICON = AirTV.icns
RC_FILE = AirTV.rc

win32 {
    LIBS += C:\\QtSDK\\mingw\\lib\\libws2_32.a
    QMAKE_LFLAGS += -static-libgcc
}
macx {
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.5
}

LIBS += -lshairplay
INCLUDEPATH += ../src/include/ ../src/bindings/qt4/
SOURCES += main.cpp\
    ../src/bindings/qt4/raopservice.cpp \
    ../src/bindings/qt4/raopcallbackhandler.cpp \
    ../src/bindings/qt4/dnssdservice.cpp \
    audiooutput.cpp \
    mainapplication.cpp \
    audiocallbacks.cpp

HEADERS  += \
    ../src/bindings/qt4/raopservice.h \
    ../src/bindings/qt4/raopcallbacks.h \
    ../src/bindings/qt4/raopcallbackhandler.h \
    ../src/bindings/qt4/dnssdservice.h \
    audiooutput.h \
    mainapplication.h \
    audiocallbacks.h

FORMS    += mainwindow.ui

RESOURCES += \
    AirTV.qrc

















