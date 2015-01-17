/*
 * (c) 2014-2015 Sam Nazarko
 * email@samnazarko.co.uk
*/
#include <QApplication>
#include "mainwindow.h"
#include <QFontDatabase>
#include <QTimer>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QFontDatabase fontDatabase;
    fontDatabase.addApplicationFont(":/assets/resources/SourceSansPro-Regular.ttf");
    MainWindow w;
    w.showFullScreen();
    /* Make sure our app installs only after starting the event loop */
    QTimer::singleShot(0, &w, SLOT(install()));
    return a.exec();
}
