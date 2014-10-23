#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QMainWindow>
#include "logger.h"
#include "utils.h"
#include <QString>
#include <QProcess>

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
    QString dev;
    bool useNetwork = false;
    bool useNFS = false;
    QString storageTypeString;
    QString storagePathString;
    bool useDHCP = true;
    QString ip;
    QString subnet;
    QString gw;
    QString dns1;
    QString dns2;

    void extract();
};
#endif
