#include "device.h"
#include "installabledevices.h"
#include <QString>

Device::Device(QString deviceName, QString devicePrefix, bool supportsPreseed)
{
    this->deviceName = deviceName;
    this->devicePrefix = devicePrefix;
    this->supportsPreseed = supportsPreseed;
    InstallableDevices::deviceCount +=1;
}
