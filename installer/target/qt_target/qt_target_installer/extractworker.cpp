#include <QFile>
#include <QTextStream>
#include "extractworker.h"

QProcess *process = NULL;

ExtractWorker::ExtractWorker(QString sourcename, QString targetname)
{
    this->sourceName = QString(sourcename);
    this->destName = QString(targetname);

}

void ExtractWorker::extract()
{
    process = new QProcess();
    connect(process, SIGNAL(readyRead()), this, SLOT(readFromProcess()));
    process->setProcessChannelMode(QProcess::MergedChannels);
    process->start("/bin/sh -c \"pv -n " + sourceName + " | tar xJf - -C " + destName);
    process->waitForFinished(-1);
    emit finished();
}

void ExtractWorker::readFromProcess()
{
    QString value = process->readAllStandardOutput();
    QString errorString = process->readAllStandardError();
    if (errorString.size() > 0)
    {
        emit error(errorString);
        return;
    }

    emit progressUpdate(value.toInt());
}
