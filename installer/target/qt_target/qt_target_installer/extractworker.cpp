#include <QFile>
#include <QTextStream>
#include <QRegExp>
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
    connect(process, SIGNAL(readyReadStandardOutput()), this, SLOT(readFromStdOut()));
    connect(process, SIGNAL(readyReadStandardError()), this, SLOT(readFromStdErr()));
    process->start("/bin/sh -c \"/usr/bin/pv -n " + sourceName + " | tar xJf - -C " + destName + "\"");
    process->waitForFinished(-1);
    if (process->exitCode() != 0)
        emit error("Extraction failed. Error: " + QString(process->readAllStandardError()));
    else
        emit finished();
}

void ExtractWorker::readFromStdOut()
{
    QString value = process->readAllStandardOutput();
    emit progressUpdate(value.toInt());
}

void ExtractWorker::readFromStdErr()
{
    QString value = process->readAllStandardError();
    QRegExp re("\\d*");  // a digit (\d), zero or more times (*)
    if (re.exactMatch(value))
        emit progressUpdate(value.toInt());
    else
        emit error(errorString);
}
