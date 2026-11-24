/*
 * (c) 2014-2015 Sam Nazarko
 * email@samnazarko.co.uk
*/
#include <QFile>
#include <QTextStream>
#include <QString>
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
    logger->addLine("Starting extract process ...");
    process = new QProcess();
    connect(process, SIGNAL(readyReadStandardOutput()), this, SLOT(readFromStdOut()));
    connect(process, SIGNAL(readyReadStandardError()), this, SLOT(readFromStdErr()));
    if (sourceName.endsWith("xz"))
    {
        logger->addLine("Extracting from " + sourceName);
        /* xz verifies the archive's integrity check, which BusyBox tar's own decompressor skips. pipefail reports a failure of pv or xz too */
        process->start("/bin/sh -c \"set -o pipefail; /usr/bin/pv -n " + sourceName + " | /usr/bin/xz -dc | tar xf - -C " + destName + "\"");
    }
    else
    {
        logger->addLine("Running " + sourceName);
        process->start("/bin/sh -c \"" + sourceName + "\"");
    }
    process->waitForFinished(-1);
    if (process->exitCode() != 0)
    {
        emit error("Extraction failed. Error: " + QString(process->readAllStandardError()));
        return;
    }
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
    /* Output arrives in pieces (xz writes its name and its message separately): only act on whole lines */
    stdErr += process->readAllStandardError();
    while (stdErr.contains('\n'))
    {
        QString value = stdErr.section('\n', 0, 0);
        stdErr.remove(0, value.length() + 1);

        bool ok;
        int i_value = value.toInt(&ok);
        if (ok)
            emit progressUpdate(i_value);
        else
        {
            process->kill();
            emit error(value);
        }
    }
}
