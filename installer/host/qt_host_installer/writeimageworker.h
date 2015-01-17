/*
 * (c) 2014-2015 Sam Nazarko
 * email@samnazarko.co.uk
*/
#ifndef WRITEIMAGEWORKER_H
#define WRITEIMAGEWORKER_H

#include <QObject>

class WriteImageWorker : public QObject
{
    Q_OBJECT

    QString deviceImage;
    QString devicePath;

public:
    explicit WriteImageWorker(QObject *parent = 0);
    WriteImageWorker(QString deviceImage, QString devicePath);
    void emitProgressUpdate(unsigned);
    void emitFlushingFS();

signals:
    void finished();
    void progressUpdate(unsigned);
    void flushingFS();
    void error();


public slots:
    void process();
};

#endif // WRITEIMAGEWORKER_H
