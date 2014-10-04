#ifndef DEVICESELECTION_H
#define DEVICESELECTION_H

#include <QWidget>
#include <QMap>
#include "diskdevice.h"

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
    void DeviceSelected(DiskDevice *nd);

private:
    Ui::DeviceSelection *ui;
    void showDevices();
    QMap<QString, DiskDevice*> devMap;
    bool installedWinImageTool;
};

#endif // DEVICESELECTION_H
