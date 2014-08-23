#ifndef INSTALLABLEDEVICES_H
#define INSTALLABLEDEVICES_H
#include "device.h"
#include <QList>

class InstallableDevices
{
public:
    static void generateDeviceList();
    static QList<Device> getDevices() { return deviceList; }
    static int deviceCount;

private:
    static QList<Device> deviceList;
};

#endif // INSTALLABLEDEVICES_H
