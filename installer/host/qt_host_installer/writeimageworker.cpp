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
#if defined(Q_OS_MAC) || defined(Q_OS_LINUX)
    success = io::writeImage(this->devicePath, this->deviceImage, this);
#endif

    if (success)
    {
        emit finished();
    }
}
