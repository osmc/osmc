/*
 * (c) 2014-2015 Sam Nazarko
 * email@samnazarko.co.uk
*/
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

void WriteImageWorker::emitProgressUpdate(unsigned progress)
{
    emit progressUpdate(progress);
}

void WriteImageWorker::emitFlushingFS() {
    emit flushingFS();
}

void WriteImageWorker::process()
{
    if (io::writeImage(this->devicePath, this->deviceImage, this))
        emit finished();
    else
        emit error();
}
