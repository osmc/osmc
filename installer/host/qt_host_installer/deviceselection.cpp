#include "deviceselection.h"
#include "ui_deviceselection.h"
#include "utils.h"
#include "io.h"
#include "diskdevice.h"
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
    devMap.clear();
    QListWidgetItem *header = new QListWidgetItem(tr("Device ID\tDevice Path\tDevice Space"), ui->devListWidget);
    header->setFlags(Qt::NoItemFlags);


#if defined(Q_OS_WIN) || defined(Q_OS_WIN32)
    if (installedWinImageTool = io::installImagingTool() == false) /* We only want to install the binary once, not every refresh */
    {
        utils::writeLog("Cannot proceed to enumerate devices");
        return;
    }
#endif
    QList<DiskDevice *> devices = io::enumerateDevice();
    for (int i = 0; i < devices.count(); i++)
    {
        DiskDevice *device = devices.at(i);
        QString deviceStr;
        if (device->getDiskPath() != "")
            deviceStr = QString::number(device->getDiskID()) + " \t " + device->getDiskPath() + " \t " + device->getDiskSize();
        else
            deviceStr = QString::number(device->getDiskID()) + " \t \t \t " + device->getDiskSize();
        QListWidgetItem *item = new QListWidgetItem(deviceStr, ui->devListWidget);
        item->setFlags(item->flags() ^ Qt::ItemIsUserCheckable);
        item->setFlags(item->flags() | Qt::ItemIsSelectable);
        devMap.insert(deviceStr, device);
    }
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
        if (item->isSelected())
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
        utils::writeLog("Device selected: " + choosenItem->text());
        emit DeviceSelected((devMap.value(choosenItem->text())));
    }
}
