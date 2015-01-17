/*
 * (c) 2014-2015 Sam Nazarko
 * email@samnazarko.co.uk
*/
#include "downloadprogress.h"
#include "ui_downloadprogress.h"
#include <QFileInfo>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QFile>
#include <QDir>
#include <QMessageBox>
#include "utils.h"
#ifdef Q_OS_LINUX
#include <unistd.h>
#endif

/* With thanks to http://qt-project.org/doc/qt-4.8/network-downloadmanager-downloadmanager-cpp.html */

DownloadProgress::DownloadProgress(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DownloadProgress)
{
    ui->setupUi(this);
}

void DownloadProgress::download(QUrl URL, bool isOnline)
{
    QString filelocation(URL.toString());

    if (isOnline == false)
    {
        /* sanitize local filename */
#if defined(Q_OS_MAC) || defined(Q_OS_LINUX)
         filelocation.replace("file://","");
#endif
        /* Emit and bail! */
        emit downloadCompleted(filelocation);
        return;
    }

    utils::writeLog("Downloading " + filelocation);
    QStringList urlSeg = filelocation.split("/");
    fileName = urlSeg.at((urlSeg.count() - 1));
    /* Do we have the file or an uncompressed version? */
    bool uncompressed = QFile(QDir::homePath() + "/" + fileName).exists();
    bool decompressed = QFile(QDir::homePath() + "/" + QString(fileName).remove(".gz")).exists();
    if (uncompressed)
    {
        if (! utils::promptYesNo(tr("Image found"), tr("Do you want to re-download this image?")))
        {
            if(decompressed) {
                if (! utils::promptYesNo(tr("Image found"), tr("Do you want to extract again?")))
                {
                    emit downloadCompleted(QDir::homePath() + "/" + QString(fileName).remove(".gz"));
                    return;
                }
            }
            emit downloadCompleted(QDir::homePath() + "/" + fileName);
            return;
        }
    }
    else if (decompressed) {
        if (! utils::promptYesNo(tr("Uncompressed Image found"), tr("Do you want to re-download this image?")))
        {
            emit downloadCompleted(QDir::homePath() + "/" + QString(fileName).remove(".gz"));
            return;
        }
    }
    fileName = QDir::homePath() + "/" + fileName;
    output.setFileName(fileName);
    if (!output.open(QIODevice::WriteOnly))
    {
        utils::writeLog("Can't open file for writing -- is it open by another process?");
        failDownload(false);
    }
    else
    {
#ifdef Q_OS_LINUX
        // Set the owner and group the same as the home path
        QFileInfo info(QDir::homePath());
        fchown(output.handle(),info.ownerId(),info.groupId());
#endif
        QNetworkRequest request(URL);
        currentDownload = manager.get(request);
        connect(currentDownload, SIGNAL(downloadProgress(qint64,qint64)),SLOT(downloadProgress(qint64,qint64)));
        connect(currentDownload, SIGNAL(finished()), SLOT(downloadFinished()));
        connect(currentDownload, SIGNAL(readyRead()), SLOT(downloadReadyRead()));
        downloadTime.start();
    }
}


void DownloadProgress::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
 {
    /* Download speed */
     double speed = bytesReceived * 1000.0 / downloadTime.elapsed();
     QString unit;
     if (speed < 1024) {
         unit = "bytes/sec";
     } else if (speed < 1024*1024) {
         speed /= 1024;
         unit = "kB/s";
     } else {
         speed /= 1024*1024;
         unit = "MB/s";
     }

     ui->downloadDetailsLabel->setText(QString::fromLatin1("%1 %2").arg(speed, 3, 'f', 1).arg(unit));
     /* Update progress */
     ui->downloadProgressBar->setMaximum(bytesTotal);
     ui->downloadProgressBar->setValue(bytesReceived);
 }

 void DownloadProgress::downloadFinished()
 {
     output.close();

     if (currentDownload->error()) {
         utils::writeLog("Error occured downloading file:");
         utils::writeLog(currentDownload->errorString());
         failDownload(true);
     } else {
         utils::writeLog("Download successful");
         emit downloadCompleted(fileName);
     }
 }

 void DownloadProgress::downloadReadyRead() { output.write(currentDownload->readAll()); }

 void DownloadProgress::failDownload(bool wasNetwork)
 {
     ui->downloadProgressBar->setValue(0);
     if (wasNetwork)
         ui->downloadDetailsLabel->setText(tr("Download failed! Please check your network connection"));
     else
         ui->downloadDetailsLabel->setText(tr("Download failed! Could not write to disk"));
 }

DownloadProgress::~DownloadProgress()
{
    delete ui;
}
