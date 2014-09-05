#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "supporteddevice.h"
#include <QString>
#include <QUrl>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include "networksettings.h"

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
    void setLanguage(QString, SupportedDevice);
    void setVersion(bool, QUrl);
    void setPreseed(int installType);
    void setNetworkInitial(bool useWireless, bool advanced);
    void setNetworkAdvanced(QString ip, QString mask, QString gw, QString dns1, QString dns2);
    void setWiFiConfiguration(QString ssid, int key_type, QString key_value);
    void showUpdate();
    void dismissUpdate();
    void replyFinished(QNetworkReply* reply);
    
private:
    Ui::MainWindow *ui;
    void translate(QString locale);
    QString language;
    QString mirrorURL;
    SupportedDevice device = SupportedDevice();
    bool isOnline;
    QUrl image;
    int installType;
    NetworkSettings *nss;
    QNetworkAccessManager *accessManager;
};

#endif // MAINWINDOW_H
