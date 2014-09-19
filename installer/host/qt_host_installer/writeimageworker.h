#ifndef WRITEIMAGEWORKER_H
#define WRITEIMAGEWORKER_H

#include <QObject>

class WriteImageWorker : public QObject
{
    Q_OBJECT
public:
    explicit WriteImageWorker(QObject *parent = 0);

signals:
    void finished();
    void progressUpdate(unsigned);
    void error();


public slots:
    void process();
};

#endif // WRITEIMAGEWORKER_H
