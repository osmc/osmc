#include "nixdiskdevice.h"

class QObject;

namespace io
{
    QList<NixDiskDevice *> enumerateDeviceLinux();
    bool writeImageLinux(QString devicePath, QString deviceImage, QObject* caller=NULL);
    bool unmountDiskLinux(QString devicePath);
    void UpdateKernelTable();
}
