#include <QString>

class QObject;
class DiskDevice;

namespace io
{
    QList<DiskDevice *> enumerateDevice();
    bool writeImage(QString devicePath, QString deviceImage, QObject* caller=NULL);

    /*!
     * \brief unmount the device denoted by path
     * \param devicePath the path to the device that should be unmounted
     * \param isDisk usually only relevant for OSX as we use a different diskutil command for discs
     * \return true if unmount was successful, otherwise false
     */
    bool unmount(QString devicePath, bool isDisk=false);

    bool mount(QString devicePath, QString mountDir);
    int getDecompressedSize(QString gzFilename);
    qint64 getFileSize(QString filename);
    bool installImagingTool();
}
