#include "deviceselection.h"
#include "ui_deviceselection.h"
#include "utils.h"
#include "io.h"
#if defined(Q_OS_LINUX) || defined(Q_OS_MAC)
#include "nixdiskdevice.h"
#endif
#include <QListWidgetItem>
#include <QMap>

DeviceSelection::DeviceSelection(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DeviceSelection)
{
    ui->setupUi(this);
    showDevices();
}

DeviceSelection::~DeviceSelection()
{
    delete ui;
}

void DeviceSelection::showDevices()
{
    ui->devListWidget->clear();
    nixdevMap.clear();
    QListWidgetItem *header = new QListWidgetItem(tr("Device ID     Device Path     Device Space"), ui->devListWidget);
    #ifdef Q_OS_MAC
    QList<NixDiskDevice *> nixdevices = io::enumerateDeviceOSX();
    #endif
    #if defined(Q_OS_WIN32) || defined(Q_OS_WIN)
    #endif
    #ifdef Q_OS_LINUX
    QList<NixDiskDevice *> nixdevices = io::enumerateDeviceLinux();
    #endif
    #if defined(Q_OS_LINUX) || defined(Q_OS_MAC)
    for (int i = 0; i < nixdevices.count(); i++)
    {
        NixDiskDevice *device = nixdevices.at(i);
        QString nixDeviceStr = QString::number(device->getDiskID()) + "     " + device->getDiskPath() + "   " + device->getDiskSize();
        QListWidgetItem *item = new QListWidgetItem(nixDeviceStr, ui->devListWidget);
        nixdevMap.insert(nixDeviceStr, device);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);
    }
    #endif
}

void DeviceSelection::on_refreshButton_clicked()
{
    showDevices();
}

void DeviceSelection::on_devicenextButton_clicked()
{
    int numItems = ui->devListWidget->count();
    QListWidgetItem *item;
    int checkCount = 0;
    for (int i = 1; i <= (numItems - 1); i++)
    {
        if (checkCount > 1)
            break;
        item = ui->devListWidget->item(i);
        if (item->checkState())
        {
            checkCount++;
        }
    }
    if (checkCount == 0)
    {
        utils::displayError(tr("Please select a device"), tr("You must select one device to image"));
    } else if (checkCount > 1)
    {
        utils::displayError(tr("Please select one device"), tr("You can only select one device to image"));
    } else {
        #if defined(Q_OS_LINUX ) || defined(Q_OS_MAC)
        utils::writeLog("Device selected: " + item->text());
        emit nixDeviceSelected((nixdevMap.value(item->text())));
        #endif
    }
}
