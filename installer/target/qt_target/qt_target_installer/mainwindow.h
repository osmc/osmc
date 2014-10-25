#ifndef MAINWINDOW_H
#define MAINWINDOW_H
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
    Ui::MainWindow *ui;
    Logger *logger;
    TargetList *targetList;
    Target *device;
    PreseedParser *preseed;
    QString installTarget;
    QString locale;
    bool useNFS = false;
    Network *nw;
    BootloaderConfig *bc;
    void extract();
};
#endif
