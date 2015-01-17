/*
 * (c) 2014-2015 Sam Nazarko
 * email@samnazarko.co.uk
*/
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "supporteddevice.h"
#include <QString>
#include <QUrl>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include "networksettings.h"
#include "diskdevice.h"
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
    int getInstallType() { return installType; }
    QString getNFSPath() { return nfsPath; }

public slots:
    void setLanguage(QString, SupportedDevice);
    void setVersion(bool, QUrl);
    void setPreseed(int installType);
    void setPreseed(int installType, QString nfsPath);
    void setNetworkInitial(bool useWireless, bool advanced);
    void setNetworkAdvanced(QString ip, QString mask, QString gw, QString dns1, QString dns2);
    void setWiFiConfiguration(QString ssid, int key_type, QString key_value);
    void selectDevice(DiskDevice *nd);
    void acceptLicense();
    void completeDownload(QString fileName);
    void showUpdate();
    void dismissUpdate();
    void replyFinished(QNetworkReply* reply);
    void rotateWidget(QWidget *oldWidget, QWidget *newWidget, bool enableBackbutton = true);
    void goBack();
    void showSuccessDialog();
    
private:
    Ui::MainWindow *ui;
    void translate(QString locale);
    QString language;
    QString mirrorURL;
    SupportedDevice device;
    QString installDevicePath;
    int installDeviceID;
    bool isOnline;
    QUrl image;
    int installType;
    NetworkSettings *nss;
    QNetworkAccessManager *accessManager;
    DiskDevice *nd;
    QString nfsPath;
    QString localeName;
    QMovie *spinner;
};

#endif // MAINWINDOW_H
