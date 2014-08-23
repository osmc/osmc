#include "installabledevices.h"
#include "device.h"
#include <QList>

void InstallableDevices::generateDeviceList()
{
    /* Generates all devices we support */
    QList<Device> *deviceList = new QList<Device>();
    Device *RaspberryPi = new Device("Raspberry Pi", "rbp", 1);
    deviceList->append(*RaspberryPi);
}
