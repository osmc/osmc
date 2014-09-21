#include <QString>
#include "nixdiskdevice.h"

namespace io
{
#if defined(Q_OS_LINUX)
     QList<NixDiskDevice *> enumerateDeviceLinux();
     QStringList enumerateDevicePartitionsLinux(QString devicePath);
     bool writeImageLinux(QString devicePath, QString deviceImage);
     bool unmountDiskLinux(QString devicePath);
#endif
#if defined(Q_OS_MAC)
     QList<NixDiskDevice *> enumerateDeviceOSX();
     bool writeImageOSX(QString devicePath, QString deviceImage);
     bool unmountDiskOSX(QString devicePath);
#endif
     int getDecompressedSize(QString gzFilename);
}
