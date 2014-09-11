#include "downloadprogress.h"
#include "ui_downloadprogress.h"
#include <QFileInfo>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QString>
#include <QUrl>
#include "utils.h"

/* With thanks to http://qt-project.org/doc/qt-4.8/network-downloadmanager-downloadmanager-cpp.html */

DownloadProgress::DownloadProgress(QWidget *parent, QUrl URL) :
    QWidget(parent),
    ui(new Ui::DownloadProgress)
{
    ui->setupUi(this);
    if (URL.isEmpty())
        /* Emit and bail! */
        emit downloadCompleted();
    else
    {
        utils::writeLog("Downloading " + URL.toString());
        output.setFileName("download.img.gz");
         if (!output.open(QIODevice::WriteOnly))
         {
             utils::writeLog("Can't open file for writing -- is it open by another process?");
             failDownload(false);
         }
         else
         {
            QNetworkRequest request(URL);
            currentDownload = manager.get(request);
            connect(currentDownload, SIGNAL(downloadProgress(qint64,qint64)),SLOT(downloadProgress(qint64,qint64)));
            connect(currentDownload, SIGNAL(finished()), SLOT(downloadFinished()));
            connect(currentDownload, SIGNAL(readyRead()), SLOT(downloadReadyRead()));
            downloadTime.start();
         }
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
         utils::writeLog("Error occured downloading file");
         failDownload(true);
     } else {
         utils::writeLog("Download successful");
         emit downloadCompleted();
     }
 }

 void DownloadProgress::downloadReadyRead()
 {
     output.write(currentDownload->readAll());
 }

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
