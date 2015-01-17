/*
 * (c) 2014-2015 Sam Nazarko
 * email@samnazarko.co.uk
*/
#include <QObject>
#include <QString>

#ifndef EXTRACTWORKER_H
#define EXTRACTWORKER_H

class ExtractWorker : public QObject {
    Q_OBJECT

    QString sourceName;
    QString destName;
public:
    ExtractWorker(QString source, QString dest);

public slots:
    void process();

signals:
    void finished();
    void progressUpdate(unsigned);
    void error();
};

#endif // EXTRACTWORKER_H
