#include "installabledevices.h"
#include "device.h"
#include <QList>

static void InstallableDevices::generateDeviceList()
{
    /* Generates all devices we support */
    deviceList = new QList<Device>();
    deviceCount = 0;
    /* Raspberry Pi support */
    Device RaspberryPi = new Device("Raspberry Pi", "rbp", 1);
    deviceList.insert(RaspberryPi);
}

