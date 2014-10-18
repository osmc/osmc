#include <QObject>
#include <QString>
#include <QProcess>

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
    void readFromProcess();

signals:
    void finished();
    void progressUpdate(unsigned);
    void error();
};

#endif // EXTRACTWORKER_H
