#include <QApplication>
#include "mainwindow.h"
#include "utils.h"
#include <QFontDatabase>
#include "installabledevices.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    #ifdef Q_OS_MAC
    utils::writeLog("OSMC Installer running on Mac OS X");
    #endif
    #ifdef Q_OS_WIN32 || #ifdef Q_OS_WIN
    utils::writeLog("OSMC Installer running on Windows");
    #endif
    #ifdef Q_OS_LINUX
    utils::writeLog("OSMC Installer running on Linux");
    #endif
    QFontDatabase fontDatabase;
    fontDatabase.addApplicationFont(":/assets/resources/SourceSansPro-Regular.ttf");
    utils::writeLog("Generating supported device list");
    InstallableDevices::generateDeviceList();
    MainWindow w;
    w.show();
    
    return a.exec();
}
