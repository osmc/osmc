#include <QString>

class QObject;
class DiskDevice;

namespace io
{
    QList<DiskDevice *> enumerateDevice();
    bool writeImage(QString devicePath, QString deviceImage, QObject* caller=NULL);
    bool unmountDisk(QString devicePath);
    int getDecompressedSize(QString gzFilename);
    qint64 getFileSize(QString filename);
    bool installImagingTool();
}
