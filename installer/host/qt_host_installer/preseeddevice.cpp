#include "preseeddevice.h"
#include "ui_preseeddevice.h"
#include "supporteddevice.h"
#include "utils.h"

PreseedDevice::PreseedDevice(QWidget *parent, SupportedDevice dev) :
    QWidget(parent),
    ui(new Ui::PreseedDevice)
{
    ui->setupUi(this);
    if (!dev.allowsPreseedingNFS() && !dev.allowsPreseedingUSB() && !dev.allowsPreseedingSD() && !dev.allowsPreseedingInternal())
    {
        utils::writeLog("This device does not support preseeding at all.");
        emit preseedSelected(utils::INSTALL_USB);
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
        emit preseedSelected(utils::INSTALL_NFS);
    }
    if (ui->emmcinstallradioButton->isChecked())
    {
        utils::writeLog("Internal storage install selected");
        emit preseedSelected(utils::INSTALL_EMMC);
    }
}
