#include <QString>
#include "nixdiskdevice.h"

namespace io
{
     QList<NixDiskDevice *> enumerateDeviceLinux();
     QList<NixDiskDevice *> enumerateDeviceOSX();
     bool writeImageOSX(QString devicePath, QString deviceImage);
     int getDecompressedSize(QString gzFilename);
}
