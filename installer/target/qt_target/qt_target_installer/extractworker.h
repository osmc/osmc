#ifdef QT_DEBUG
#include <QDebug>
#endif

#include <QObject>
#include <QProcess>
#include <QString>

#ifndef EXTRACTWORKER_H
#define EXTRACTWORKER_H

class ExtractWorker : public QObject {
    Q_OBJECT

    QString sourceName;
    QString destName;

public:
    ExtractWorker(QString source, QString dest, QObject* parent = NULL);

public slots:
    void extract();
    void readFromProcess();

signals:
    void finished();
    void progressUpdate(unsigned);
    void error(QString error);

private:
    QProcess *process = NULL;
};

#endif // EXTRACTWORKER_H
