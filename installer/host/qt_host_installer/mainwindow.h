#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "supporteddevice.h"
#include <QString>
#include <QUrl>
#include <QNetworkReply>
#include <QNetworkAccessManager>

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
    void replyFinished(QNetworkReply* reply);
    
private:
    Ui::MainWindow *ui;
    void translate(QString locale);
    QString language;
    QString mirrorURL;
    SupportedDevice *device;
    bool isOnline;
    QUrl image;
    QNetworkAccessManager *accessManager;
};

#endif // MAINWINDOW_H
