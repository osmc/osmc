/*
 * (c) 2014-2015 Sam Nazarko
 * email@samnazarko.co.uk
*/
#ifndef DISKDEVICE_H
#define DISKDEVICE_H
#include <QString>

class DiskDevice
{
public:
    DiskDevice(int diskID, QString diskPath, QString diskSize, QString label="");
    int getDiskID() { return diskID; }
    QString getDiskPath() { return diskPath; }
    QString getDiskSize() { return diskSize; }
    QString getLabel() { return diskLabel; }
    void setLabel(QString label) { diskLabel = label; }
    void setIsWritable(bool isWritable) { this->isWritable= isWritable; }
    bool getIsWritable() { return isWritable; }

private:
    int diskID;
    QString diskPath;
    QString diskSize;
    QString diskLabel;
    bool isWritable;
};

#endif // DISKDEVICE_H
