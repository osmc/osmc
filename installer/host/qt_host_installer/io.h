#include <QString>

class QObject;
class DiskDevice;

namespace io
{
    QList<DiskDevice *> enumerateDevice();
    bool isUsbDevice(QString devicePath);
    bool writeImage(QString devicePath, QString deviceImage, QObject* caller=NULL);
    bool unmount(QString devicePath, bool isDisk = false);
    void updateKernelTable();
    bool mount(QString devicePath, QString mountDir);
    int getDecompressedSize(QString gzFilename);
    qint64 getFileSize(QString filename);
    bool installImagingTool();
}
