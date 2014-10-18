#include <QDebug>
#include <QTextStream>
#include "extractworker.h"

QProcess *myProcess = NULL;

ExtractWorker::ExtractWorker(QString sourcename, QString targetname)
{
    this->sourceName = QString(sourcename);
    this->destName = QString(targetname);

}

void ExtractWorker::process()
{
    myProcess = new QProcess();
    connect(myProcess, SIGNAL(readyRead()), this, SLOT(readFromProcess()));
    myProcess->setProcessChannelMode(QProcess::MergedChannels);
    myProcess->start("/bin/sh -c \"pv -n " + sourceName + " | tar xJf - -C " + destName);

    myProcess->waitForFinished(-1);
    qDebug() << "exitCodePv: " << myProcess->exitCode();
    qDebug() << myProcess->readAllStandardError();
    qDebug() << myProcess->readAllStandardOutput();

    emit finished();
}

void ExtractWorker::readFromProcess()
{
    QString value = myProcess->readAll();
    qDebug() << "signal to read value";
    qDebug() << "readAll: " << value;
    qDebug() << "assuming intvalue: " << value.toInt();
    qDebug() << "assuming longvalue: " << value.toLong();
    qDebug() << "standartOut: " << myProcess->readAllStandardOutput();
    if (myProcess->readAllStandardError().size() > 0)
    {
        emit error();
        return;
    }

    emit progressUpdate(value.toInt());
}
