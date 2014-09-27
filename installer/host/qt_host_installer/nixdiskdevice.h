#ifndef NIXDISKDEVICE_H
#define NIXDISKDEVICE_H
#include <QString>

class NixDiskDevice
{
public:
    NixDiskDevice(int diskID, QString diskPath, QString diskSize, bool removable = false, bool sdCard = false, bool usb = false);
    int getDiskID() { return diskID; }
    QString getDiskPath() { return diskPath; }
    QString getDiskSize() { return diskSize; }
    bool removable;
    bool sdCard;
    bool usb;

private:
    int diskID;
    QString diskPath;
    QString diskSize;
};

#endif // NIXDISKDEVICE_H
