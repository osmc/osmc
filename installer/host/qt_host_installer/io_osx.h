#include "nixdiskdevice.h"

namespace io
{
    QList<NixDiskDevice *> enumerateDeviceOSX();
    bool writeImageOSX(QString devicePath, QString deviceImage);
    bool unmountDiskOSX(QString devicePath);
}
