#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "supporteddevice.h"
#include <QString>
#include <QUrl>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include "networksettings.h"
#include "nixdiskdevice.h"
#include <QMovie>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    QList<QWidget *> widgetList;
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    SupportedDevice *getSupportedDevice();
    int getInstallType();

public slots:
    void setLanguage(QString, SupportedDevice);
    void setVersion(bool, QUrl);
    void setPreseed(int installType);
    void setNetworkInitial(bool useWireless, bool advanced);
    void setNetworkAdvanced(QString ip, QString mask, QString gw, QString dns1, QString dns2);
    void setWiFiConfiguration(QString ssid, int key_type, QString key_value);
    void selectNixDevice(NixDiskDevice *nd);
    void acceptLicense();
    void completeDownload(QString fileName);
    void showUpdate();
    void dismissUpdate();
    void replyFinished(QNetworkReply* reply);
    void rotateWidget(QWidget *oldWidget, QWidget *newWidget, bool enableBackbutton = true);
    void goBack();
    
private:
    Ui::MainWindow *ui;
    void translate(QString locale);
    QString language;
    QString mirrorURL;
    SupportedDevice device;
    QString installDevicePath;
    bool isOnline;
    QUrl image;
    int installType;
    NetworkSettings *nss;
    QNetworkAccessManager *accessManager;
    #if defined(Q_OS_LINUX) || defined(Q_OS_MAC)
    NixDiskDevice *nd;
    #endif
    QMovie *spinner;
};

#endif // MAINWINDOW_H
