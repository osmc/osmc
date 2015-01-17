/*
 * (c) 2014-2015 Sam Nazarko
 * email@samnazarko.co.uk
*/
#include "diskdevice.h"
#include "utils.h"

DiskDevice::DiskDevice(int diskID, QString diskPath, QString diskSize)
{
    utils::writeLog("New disk device entry created with entry point " + diskPath + " and " + diskSize + " free space");
    this->diskID = diskID;
    this->diskPath = diskPath;
    this->diskSize = diskSize;
}
