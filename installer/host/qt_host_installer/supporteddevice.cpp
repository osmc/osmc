#include "supporteddevice.h"
#include <QString>

SupportedDevice::SupportedDevice(QString dName, QString dsName, bool preseedNetwork, bool preseedNFS, bool preseedUSB, bool preseedPartitioning)
{
    deviceName = dName;
    deviceShortName = dsName;
    preseedNetwork = preseedNetwork;
    preseedNFS = preseedNFS;
    preseedUSB = preseedUSB;
    preseedPartitioning = preseedPartitioning;
}
