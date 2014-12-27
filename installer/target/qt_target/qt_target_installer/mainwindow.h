#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QMainWindow>
#include "logger.h"
#include "utils.h"
#include <QString>
#include <QProcess>
#include <QLabel>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QSize>
#include "targetlist.h"
#include "target.h"
#include "preseedparser.h"
#include "network.h"
#include "bootloaderconfig.h"
#include <QFont>

namespace Ui {
class MainWindow;
}

class MainWindow : public QWidget
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
    void setupSizeAndBackground();
    QFont getFont(QString refString, QWidget* element, int refPointSize);
    void setupStatusLabel();
    void setupProgressBar();
    void setupCopyrightLabel();

    /* Ui */
    int appWidth;
    int appHeight;
    QSize size;
    QVBoxLayout* layout;
    QLabel* placeholder;
    QLabel* statusLabel;
    QLabel* copyrightLabel;
    QProgressBar* progressBar;

    /* internal */
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
