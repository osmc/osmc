/*
 * (c) 2014-2015 Sam Nazarko
 * email@samnazarko.co.uk
*/
#include "supporteddevice.h"
#include <QString>

SupportedDevice::SupportedDevice()
{
}

SupportedDevice::SupportedDevice(QString dName, QString dsName, bool preseedNetwork, bool preseedNFS, bool preseedUSB, bool preseedInternal, bool preseedSD, bool preseedPartitioning)
{
    this->deviceName = dName;
    this->deviceShortName = dsName;
    this->preseedNetwork = preseedNetwork;
    this->preseedNFS = preseedNFS;
    this->preseedUSB = preseedUSB;
    this->preseedInternal = preseedInternal;
    this->preseedSD = preseedSD;
    this->preseedPartitioning = preseedPartitioning;
}
