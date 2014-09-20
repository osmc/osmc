#include "writeimageworker.h"
#include "io.h"

WriteImageWorker::WriteImageWorker(QObject *parent) :
    QObject(parent)
{
}

WriteImageWorker::WriteImageWorker(QString deviceImage, QString devicePath)
{
    this->deviceImage = QString(deviceImage);
    this->devicePath = QString(devicePath);
}


void WriteImageWorker::process()
{

    bool success = false;
#ifdef Q_OS_MAC
    success = io::writeImageOSX(this->devicePath, this->deviceImage);
#endif

    if (success)
    {
        emit finished();
    }
}
