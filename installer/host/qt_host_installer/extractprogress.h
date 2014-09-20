#ifndef EXTRACTPROGRESS_H
#define EXTRACTPROGRESS_H

#include <QWidget>
#include <QString>
#include <QProcess>

namespace Ui {
class ExtractProgress;
}

class ExtractProgress : public QWidget
{
    Q_OBJECT
    qint64 decompressedSize;
    QString devicePath;
    QString deviceImage;
    QProcess p;
    
public:
    explicit ExtractProgress(QWidget *parent = 0, QString devicePath = NULL, QString deviceImage = NULL );
    ~ExtractProgress();

public slots:
    void extract();
    void extractError();
    void writeError();
    void setProgress(unsigned);
    void finished();
    void writeFinished();

private:
    Ui::ExtractProgress *ui;
    void doExtraction();
    void writeImageToDisk();
    bool unmountDisk();

};

#endif // EXTRACTPROGRESS_H
