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
    ~MainWindow();

public slots:
    void install();
    void setProgress(unsigned value);
    void finished();

private:
    Ui::MainWindow *ui;
    Logger *logger;
    QString dev;
    void haltInstall(QString errorMsg);
    void preseed();
};
#endif
