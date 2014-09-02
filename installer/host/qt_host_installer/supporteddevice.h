#ifndef SUPPORTEDDEVICE_H
#define SUPPORTEDDEVICE_H
#include <QString>

class SupportedDevice
{
public:
    SupportedDevice(QString dName, QString dsName, bool preseedNetwork, bool preseedNFS, bool preseedUSB, bool preseedPartitioning);
    QString getDeviceName() { return deviceName; }
    QString getDeviceShortName() { return deviceShortName; }
    bool allowsPreseedingNetwork() { return preseedNetwork; }
    bool allowsPreseedingNFS() { return preseedNFS; }
    bool allowsPreseedingUSB() { return preseedUSB; }
    bool allowsPreseedingPartitioning() { return preseedPartitioning; }
private:
    QString deviceName;
    QString deviceShortName;
    bool preseedNetwork;
    bool preseedNFS;
    bool preseedUSB;
    bool preseedPartitioning;
};

#endif // SUPPORTEDDEVICE_H
