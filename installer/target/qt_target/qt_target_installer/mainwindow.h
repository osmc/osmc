/*
 * (c) 2014-2015 Sam Nazarko
 * email@samnazarko.co.uk
*/
#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#define FONT_STATUSLABEL_RATIO 3.4
#define FONT_PROGRESSBAR_RATIO 3.6
#include <QMainWindow>
#include "logger.h"
#include "utils.h"
#include <QString>
#include <QProcess>
#include "targetlist.h"
#include "target.h"
#include "preseedparser.h"
#include "network.h"
#include "bootloaderconfig.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    void dumpLog();
    ~MainWindow();

public slots:
    void install();
    void setupBootLoader();
    void setProgress(unsigned value);
    void haltInstall(QString errorMsg);
    void finished();

private:
    void extract();
    QFont getFont(QWidget* element, float ratio);

    Ui::MainWindow *ui;
    Logger *logger;
    TargetList *targetList;
    Target *device;
    Utils *utils;
    PreseedParser *preseed;
    QString installTarget;
    QString locale;
    bool useNFS = false;
    Network *nw;
    BootloaderConfig *bc;
};
#endif
