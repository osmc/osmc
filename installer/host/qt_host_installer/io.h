#include <QString>
#include "nixdiskdevice.h"

namespace io
{
     QList<NixDiskDevice *> enumerateDeviceLinux();
     QList<NixDiskDevice *> enumerateDeviceOSX();
}
