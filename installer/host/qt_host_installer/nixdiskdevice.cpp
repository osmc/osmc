#include "nixdiskdevice.h"
#include "utils.h"

NixDiskDevice::NixDiskDevice(int diskID, QString diskPath, QString diskSize)
{
    utils::writeLog("New UNIX disk device entry created with entry point " + diskPath + " and " + diskSize + " free space");
    this->diskID = diskID;
    this->diskPath = diskPath;
    this->diskSize = diskSize;
}
