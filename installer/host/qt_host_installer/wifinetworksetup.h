/*
 * (c) 2014-2015 Sam Nazarko
 * email@samnazarko.co.uk
*/
#ifndef WIFINETWORKSETUP_H
#define WIFINETWORKSETUP_H

#include <QWidget>
#include "ui_wifinetworksetup.h"

namespace Ui {
class WiFiNetworkSetup;
}

class WiFiNetworkSetup : public QWidget
{
    Q_OBJECT

    virtual void showEvent(QShowEvent *event)
    {
        ui->networkoptionsnextButton->setEnabled(true);
    }

public:
    explicit WiFiNetworkSetup(QWidget *parent = 0);
    ~WiFiNetworkSetup();
    
private slots:
    void on_networkoptionsnextButton_clicked();

    void on_keySelectionBox_currentIndexChanged(const QString &arg1);

signals:
    void wifiNetworkConfigured(QString ssid, int key_type, QString key_value);

private:
    Ui::WiFiNetworkSetup *ui;
};

#endif // WIFINETWORKSETUP_H
