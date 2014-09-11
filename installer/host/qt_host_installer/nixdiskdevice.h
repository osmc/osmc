#ifndef NIXDISKDEVICE_H
#define NIXDISKDEVICE_H
#include <QString>

class NixDiskDevice
{
public:
    NixDiskDevice(int diskID, QString diskPath, QString diskSize);
    int getDiskID() { return diskID; }
    QString getDiskPath() { return diskPath; }
    QString getDiskSize() { return diskSize; }

private:
    int diskID;
    QString diskPath;
    QString diskSize;
};

#endif // NIXDISKDEVICE_H
