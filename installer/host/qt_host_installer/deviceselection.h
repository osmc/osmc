#ifndef DEVICESELECTION_H
#define DEVICESELECTION_H

#include <QWidget>
#include <QMap>
#include "nixdiskdevice.h"

namespace Ui {
class DeviceSelection;
}

class DeviceSelection : public QWidget
{
    Q_OBJECT
    
public:
    explicit DeviceSelection(QWidget *parent = 0);
    ~DeviceSelection();
    
private slots:
    void on_refreshButton_clicked();

    void on_devicenextButton_clicked();

signals:
    void nixDeviceSelected(NixDiskDevice *nd);

private:
    Ui::DeviceSelection *ui;
    void showDevices();
    QMap<QString, NixDiskDevice*>  nixdevMap;
};

#endif // DEVICESELECTION_H
