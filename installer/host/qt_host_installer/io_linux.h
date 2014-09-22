#include "nixdiskdevice.h"

namespace io
{
    QList<NixDiskDevice *> enumerateDeviceLinux();
    bool writeImageLinux(QString devicePath, QString deviceImage);
    bool unmountDiskLinux(QString devicePath);
    void UpdateKernelTable();
}
