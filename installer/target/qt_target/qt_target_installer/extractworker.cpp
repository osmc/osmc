#include <QFile>
#include <QTextStream>
#include "extractworker.h"

ExtractWorker::ExtractWorker(QString sourcename, QString targetname, Logger *logger, QObject* parent):
    QObject(parent)
{
    this->sourceName = QString(sourcename);
    this->destName = QString(targetname);
    this->logger = logger;
}

void ExtractWorker::extract()
{
    logger->addLine("Starting extract progress...");
    process = new QProcess();
    connect(process, SIGNAL(readyRead()), this, SLOT(readFromProcess()));
    process->start("/bin/sh -c \"/usr/bin/pv -n " + sourceName + " | tar xJf - -C " + destName + "\"");
    process->waitForFinished(-1);
    if (process->exitCode() != 0)
        emit error("Extraction failed. Error: " + QString(process->readAllStandardError()));
    else
        emit finished();
}

void ExtractWorker::readFromProcess()
{
    QString value = process->readAllStandardOutput();
    QString errorString = process->readAllStandardError();
    if (errorString.size() > 0)
        emit error(errorString);
    else
        emit progressUpdate(value.toInt());
}
