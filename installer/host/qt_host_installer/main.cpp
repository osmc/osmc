/*
 * (c) 2014-2015 Sam Nazarko
 * email@samnazarko.co.uk
*/
#include <QApplication>
#include "mainwindow.h"
#include "utils.h"
#include <QFontDatabase>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    #ifdef Q_OS_MAC
    utils::writeLog("OSMC Installer running on Mac OS X");
    #endif
    #if defined(Q_OS_WIN) || defined(Q_OS_WIN32)
    utils::writeLog("OSMC Installer running on Windows");
    #endif
    #ifdef Q_OS_LINUX
    utils::writeLog("OSMC Installer running on Linux");
    #endif
    QFontDatabase fontDatabase;
    fontDatabase.addApplicationFont(":/assets/resources/SourceSansPro-Regular.ttf");
    MainWindow w;
    w.show();
    #ifdef Q_OS_MAC
    w.raise();
    #endif
    w.activateWindow();
    return a.exec();
}
