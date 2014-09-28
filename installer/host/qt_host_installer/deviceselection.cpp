#include "deviceselection.h"
#include "ui_deviceselection.h"
#include "mainwindow.h"
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
    int installType = -1;
    if(this->parent()) {
        MainWindow* mw;
        if(mw =qobject_cast<MainWindow*>(parent())) {
            installType = mw->getInstallType();
        }
    }
    QListWidgetItem *header = new QListWidgetItem(tr("Device ID     Device Path     Device Space"), ui->devListWidget);

    #if defined(Q_OS_LINUX) || defined(Q_OS_MAC)
    QList<NixDiskDevice *> nixdevices = io::enumerateDevice();
    for (int i = 0; i < nixdevices.count(); i++)
    {
        NixDiskDevice *device = nixdevices.at(i);
        QString nixDeviceStr = QString::number(device->getDiskID()) + "     " + device->getDiskPath() + "   " + device->getDiskSize();
        QListWidgetItem *item = new QListWidgetItem(nixDeviceStr, ui->devListWidget);
        nixdevMap.insert(nixDeviceStr, device);
        #ifdef Q_OS_LINUX
        // only set the right devices checkable
        switch(installType) {
        case utils::INSTALL_SD:
            if(device->sdCard){
                item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            }
            else {
                item->setFlags(item->flags() ^ Qt::ItemIsUserCheckable);
            }
            break;
        case utils::INSTALL_USB:
            if(device->usb){
                item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            }
            else {
                item->setFlags(item->flags() ^ Qt::ItemIsUserCheckable);
            }
            break;
        default:
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            break;
        }
        #else
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        #endif
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
    QListWidgetItem *choosenItem=NULL;
    int checkCount = 0;
    for (int i = 0; i <= (numItems-1); i++)
    {
        if (checkCount > 1)
            break;
        QListWidgetItem* item = ui->devListWidget->item(i);
        if (item->checkState())
        {
            if(!choosenItem) {
                choosenItem = item;
            }
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
        utils::writeLog("Device selected: " + choosenItem->text());
        emit nixDeviceSelected((nixdevMap.value(choosenItem->text())));
        #endif
    }
}
