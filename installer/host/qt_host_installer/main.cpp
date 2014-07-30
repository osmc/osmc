#include <QApplication>
#include "mainwindow.h"
#include "utils.h"

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
    QString locale = QLocale::system().name();
    utils::writeLog("Detected locale as " + locale);
    utils::loadTranslation(locale, &a);
    MainWindow w;
    w.show();
    
    return a.exec();
}
