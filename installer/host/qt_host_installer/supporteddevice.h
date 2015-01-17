/*
 * (c) 2014-2015 Sam Nazarko
 * email@samnazarko.co.uk
*/
#ifndef SUPPORTEDDEVICE_H
#define SUPPORTEDDEVICE_H
#include <QString>

class SupportedDevice
{
public:
    SupportedDevice();
    SupportedDevice(QString dName, QString dsName, bool preseedNetwork, bool preseedNFS, bool preseedUSB, bool preseedInternal, bool preseedSD, bool preseedPartitioning);
    QString getDeviceName() { return deviceName; }
    QString getDeviceShortName() { return deviceShortName; }
    bool allowsPreseedingNetwork() { return preseedNetwork; }
    bool allowsPreseedingNFS() { return preseedNFS; }
    bool allowsPreseedingUSB() { return preseedUSB; }
    bool allowsPreseedingInternal() { return preseedInternal; }
    bool allowsPreseedingSD() { return preseedSD; }
    bool allowsPreseedingPartitioning() { return preseedPartitioning; }
private:
    QString deviceName;
    QString deviceShortName;
    bool preseedNetwork;
    bool preseedNFS;
    bool preseedUSB;
    bool preseedInternal;
    bool preseedSD;
    bool preseedPartitioning;
};

#endif // SUPPORTEDDEVICE_H
