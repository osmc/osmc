#include <QApplication>
#include "mainwindow.h"
#include <QFontDatabase>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QFontDatabase fontDatabase;
    fontDatabase.addApplicationFont(":/assets/resources/SourceSansPro-Regular.ttf");
    MainWindow w;
    w.show();
    w.install();
    return a.exec();
}
