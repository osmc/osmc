/*
 * (c) 2014-2015 Sam Nazarko
 * email@samnazarko.co.uk
*/
#include "diskdevice.h"
#include "utils.h"

DiskDevice::DiskDevice(int diskID, QString diskPath, QString diskSize, QString diskLabel)
{
    utils::writeLog("New disk device entry created with entry point " + diskPath + ", " + diskSize + " free space and label " + diskLabel);
    this->diskID = diskID;
    this->diskPath = diskPath;
    this->diskSize = diskSize;
    this->diskLabel = diskLabel;
    this->isWritable = false;
}
