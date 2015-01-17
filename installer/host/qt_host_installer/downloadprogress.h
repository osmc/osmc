/*
 * (c) 2014-2015 Sam Nazarko
 * email@samnazarko.co.uk
*/
#ifndef DOWNLOADPROGRESS_H
#define DOWNLOADPROGRESS_H

#include <QWidget>
#include <QString>
#include <QNetworkAccessManager>
#include <QFile>
#include <QTime>
#include <QUrl>

namespace Ui {
class DownloadProgress;
}

class DownloadProgress : public QWidget
{
    Q_OBJECT
    
public:
    explicit DownloadProgress(QWidget *parent = 0);
    ~DownloadProgress();

public slots:
    void download(QUrl URL, bool isOnline);

signals:
    void downloadCompleted(QString);

private slots:
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void downloadFinished();
    void downloadReadyRead();
    
private:
    Ui::DownloadProgress *ui;
    QNetworkAccessManager manager;
    QNetworkReply *currentDownload;
    QFile output;
    QTime downloadTime;
    QString fileName;
    void failDownload(bool wasNetwork);
};

#endif // DOWNLOADPROGRESS_H
