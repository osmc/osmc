/*
 * (c) 2014-2015 Sam Nazarko
 * email@samnazarko.co.uk
*/
#include "preseeddevice.h"
#include "supporteddevice.h"
#include "utils.h"
#include <QInputDialog>
#include <QString>

PreseedDevice::PreseedDevice(QWidget *parent, SupportedDevice dev) :
    QWidget(parent),
    ui(new Ui::PreseedDevice)
{
    ui->setupUi(this);
    if (!dev.allowsPreseedingNFS() && !dev.allowsPreseedingUSB() && !dev.allowsPreseedingSD() && !dev.allowsPreseedingInternal())
    {
        utils::writeLog("This device does not support preseeding at all.");
        emit preseedSelected(utils::INSTALL_NOPRESEED);
    }
    if (!dev.allowsPreseedingNFS() && !dev.allowsPreseedingUSB() && !dev.allowsPreseedingSD() && !dev.allowsPreseedingInternal() && dev.allowsPreseedingPartitioning())
    {
        utils::writeLog("This device is a traditional partitionable target, i.e. desktop");
        emit preseedSelected(utils::INSTALL_PARTITIONER);
    }
    if (!dev.allowsPreseedingNFS())
    {
        utils::writeLog("Disabling NFS install for device " + dev.getDeviceName() + " as it does not support it");
        ui->nfsinstallradioButton->setEnabled(false);
    }
    if (!dev.allowsPreseedingUSB())
    {
        utils::writeLog("Disabling USB install for device " + dev.getDeviceName() + " as it does not support it");
        ui->usbinstallradioButton->setEnabled(false);
    }
    if (!dev.allowsPreseedingSD())
    {
        utils::writeLog("Disabling SD install for device " + dev.getDeviceName() + " as it does not support it");
        ui->sdinstallradioButton->setEnabled(false);
    }
    if (!dev.allowsPreseedingInternal())
    {
        utils::writeLog("Disabling internal install for device " + dev.getDeviceName() + " as it does not support it");
        ui->emmcinstallradioButton->setEnabled(false);
    }
}

PreseedDevice::~PreseedDevice()
{
    delete ui;
}

void PreseedDevice::on_installoptionsnextButton_clicked()
{
    ui->installoptionsnextButton->setEnabled(false);

    if (ui->sdinstallradioButton->isChecked())
    {
        utils::writeLog("SD installation selected");
        emit preseedSelected(utils::INSTALL_SD);
    }
    if (ui->usbinstallradioButton->isChecked())
    {
        utils::writeLog("USB installation selected");
        emit preseedSelected(utils::INSTALL_USB);
    }
    if (ui->nfsinstallradioButton->isChecked())
    {
        utils::writeLog("NFS installation selected");
        bool response;
        QString nfsPath = QInputDialog::getText(this, tr("NFS install"), tr("Please specify path to NFS share"), QLineEdit::Normal, "192.168.1.100:/osmc", &response);
        if (response && !nfsPath.isEmpty())
            emit preseedSelected(utils::INSTALL_NFS, nfsPath);
        else
        {
            ui->sdinstallradioButton->setChecked(true);
            ui->installoptionsnextButton->setEnabled(true);
        }
    }
    if (ui->emmcinstallradioButton->isChecked())
    {
        utils::writeLog("Internal storage install selected");
        emit preseedSelected(utils::INSTALL_EMMC);
    }        
}
