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
    enum ExtractionProgressStatus {
        EXTRACTING_STATUS,
        WRITING_STATUS
    };
    
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

signals:
    void finishedExtraction();

private:
    Ui::ExtractProgress *ui;
    void doExtraction();
    void writeImageToDisk();
    bool unmountDisk();

    ExtractionProgressStatus status;
};

#endif // EXTRACTPROGRESS_H
