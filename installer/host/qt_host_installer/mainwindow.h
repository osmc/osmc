#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "supporteddevice.h"
#include <QString>
#include <QUrl>

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
    void setLanguage(QString, SupportedDevice*);
    void setVersion(bool, QUrl);
    void showUpdate();
    void dismissUpdate();
    
private:
    Ui::MainWindow *ui;
    void translate(QString locale);
    QString language;
    SupportedDevice *device;
    bool isOnline;
    QUrl image;
};

#endif // MAINWINDOW_H
