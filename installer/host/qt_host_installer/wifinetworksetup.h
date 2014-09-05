#ifndef WIFINETWORKSETUP_H
#define WIFINETWORKSETUP_H

#include <QWidget>

namespace Ui {
class WiFiNetworkSetup;
}

class WiFiNetworkSetup : public QWidget
{
    Q_OBJECT
    
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
