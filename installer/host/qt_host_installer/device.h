#ifndef DEVICE_H
#define DEVICE_H

class Device
{
public:
    QString deviceName;
    QString devicePrefix;
    bool supportsPreseed;
    Device(QString deviceName, QString devicePrefix, bool supportsPreseed);
    QString getDeviceName() { return deviceName; }
    QString getDevicePrefix() { return devicePrefix; }
    bool getSupportsPreseed() { return supportsPreseed; }
};

#endif // DEVICE_H
