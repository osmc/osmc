#ifndef DISKDEVICE_H
#define DISKDEVICE_H
#include <QString>

class DiskDevice
{
public:
    DiskDevice(int diskID, QString diskPath, QString diskSize);
    int getDiskID() { return diskID; }
    QString getDiskPath() { return diskPath; }
    QString getDiskSize() { return diskSize; }

private:
    int diskID;
    QString diskPath;
    QString diskSize;
};

#endif // DISKDEVICE_H
