/*
 * (c) 2014-2015 Sam Nazarko
 * email@samnazarko.co.uk
*/
#ifdef QT_DEBUG
#include <QDebug>
#endif

#include <QObject>
#include <QProcess>
#include <QString>
#include "logger.h"

#ifndef EXTRACTWORKER_H
#define EXTRACTWORKER_H

class ExtractWorker : public QObject {
    Q_OBJECT

public:
    ExtractWorker(QString source, QString dest, Logger *logger, QObject* parent = NULL);

public slots:
    void extract();
    void readFromStdOut();
    void readFromStdErr();

signals:
    void finished();
    void progressUpdate(unsigned);
    void error(QString error);

private:
    QProcess *process = NULL;
    QString sourceName;
    QString destName;
    Logger *logger;

};

#endif // EXTRACTWORKER_H
