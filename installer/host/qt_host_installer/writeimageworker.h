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

signals:
    void finished();
    void progressUpdate(unsigned);
    void error();


public slots:
    void process();
};

#endif // WRITEIMAGEWORKER_H
