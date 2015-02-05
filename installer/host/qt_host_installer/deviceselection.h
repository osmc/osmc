/*
 * (c) 2014-2015 Sam Nazarko
 * email@samnazarko.co.uk
*/
#ifndef DEVICESELECTION_H
#define DEVICESELECTION_H

#include <QWidget>
#include <QMap>
#include "diskdevice.h"
#include "ui_deviceselection.h"

namespace Ui {
class DeviceSelection;
}

class DeviceSelection : public QWidget
{
    Q_OBJECT

    virtual void showEvent(QShowEvent *event)
    {
        ui->devicenextButton->setEnabled(true);
        ui->refreshButton->setEnabled(true);
    }

public:
    explicit DeviceSelection(QWidget *parent = 0);
    ~DeviceSelection();
    
private slots:
    void on_refreshButton_clicked();

    void on_devicenextButton_clicked();

signals:
    void DeviceSelected(DiskDevice *nd);

private:
    Ui::DeviceSelection *ui;
    void showDevices();
    QMap<QString, DiskDevice*> devMap;
    bool installedWinImageTool;
};

#endif // DEVICESELECTION_H
