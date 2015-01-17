/*
 * (c) 2014-2015 Sam Nazarko
 * email@samnazarko.co.uk
*/
#include "extractprogress.h"
#include "ui_extractprogress.h"
#include <QString>
#include "utils.h"
#include "io.h"
#include "extractworker.h"
#include "writeimageworker.h"
#include <QThread>
#include <QProcess>

#define SET_BINARY_MODE(file)

ExtractProgress::ExtractProgress(QWidget *parent, QString devicePath, QString deviceImage):
    QWidget(parent),
    ui(new Ui::ExtractProgress)
{
    ui->setupUi(this);

    ui->extractProgressBar->setMaximum(0);
    ui->extractProgressBar->setMinimum(0);

    this->devicePath = QString(devicePath);

    /* Windows appends file:// */
    this->deviceImage = QString(deviceImage).remove("file:///");

}

void ExtractProgress::extract()
{
    if (deviceImage.endsWith(".gz"))
    {
        ui->extractProgressBar->setMaximum(io::getDecompressedSize(deviceImage));
        ui->extractProgressBar->setMinimum(0);
        doExtraction();
        deviceImage.remove(".gz");
    }
    else if (deviceImage.endsWith(".img"))
    {
        utils::writeLog("File claims to be already an 'img'. No need to extract.");
        writeImageToDisk();
    }

}

void ExtractProgress::writeImageToDisk()
{
    status = WRITING_STATUS;
    ui->extractProgressBar->setMaximum(io::getFileSize(deviceImage));
    ui->extractProgressBar->setMinimum(0);
#if defined (Q_OS_MAC) || defined(Q_OS_LINUX)
    ui->extractDetailsLabel->setText(tr("Unmounting") + " " + this->devicePath + tr("\n(This might take a few seconds!)"));
#endif
#if defined (Q_OS_WIN) || defined(Q_OS_WIN32)
    ui->extractDetailsLabel->setText(tr("Unmounting device"));
#endif
    utils::writeLog("Requesting confirmation from user");
    /* We don't always have drive letter on Windows, plus, we use ID not disk */

#if defined(Q_OS_WIN) || defined(Q_OS_WIN32)
    QString message = tr("Are you sure you want to install OSMC on the device you selected?") + "\n" + tr("OSMC is not responsible for loss of personal data");
#endif
#if defined (Q_OS_MAC) || defined(Q_OS_LINUX)
    QString message = tr("Are you sure you want to install OSMC on ") + this->devicePath + "?\n" + tr("OSMC is not responsible for loss of personal data");
#endif
    if (utils::promptYesNo(tr("Are you sure"), message))
    {
        utils::writeLog("User confirmed");
        bool unmountSuccess = unmountDisk();

        if (unmountSuccess == false)
        {
            utils::displayError(tr("Unmount failed!"), tr("Could not unmount device ") + devicePath + "." + tr("Check the log for error messages. OSMC must exit now."), true);
            QApplication::quit();
            return;
        }
#if defined (Q_OS_MAC) || defined(Q_OS_LINUX)
        ui->extractDetailsLabel->setText(tr("Writing image to ") + this->devicePath + "\n" + tr("Please be patient"));
#endif
#if defined (Q_OS_WIN) || defined (Q_OS_WIN32)
        ui->extractDetailsLabel->setText(tr(("Writing image to your device")));
#endif

#if defined (Q_OS_MAC) || defined (Q_OS_WIN) || defined (Q_OS_WIN32)
        /* At the moment, We can't provide a real progress bar on OSX or Windows, so set up a busy bar here */
        ui->extractProgressBar->setMaximum(0);
        ui->extractProgressBar->setMinimum(0);
#endif

        QThread* thread = new QThread;
        WriteImageWorker *worker = new WriteImageWorker(this->deviceImage, this->devicePath);
        worker->moveToThread(thread);
        connect(worker, SIGNAL(error()), this, SLOT(writeError()));
        connect(thread, SIGNAL(started()), worker, SLOT(process()));
        connect(worker, SIGNAL(flushingFS()), this, SLOT(setFlushing()));
        connect(worker, SIGNAL(progressUpdate(unsigned)), this, SLOT(setProgress(unsigned)));
        connect(worker, SIGNAL(finished()), thread, SLOT(quit()));
        connect(worker, SIGNAL(finished()), worker, SLOT(deleteLater()));
        connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
        connect(thread, SIGNAL(finished()), this, SLOT(writeFinished()));
        thread->start();
    }
    else
    {
        utils::writeLog("User decided not to write to the disk");
        QApplication::quit();
    }
}


bool ExtractProgress::unmountDisk()
{
#if defined(Q_OS_MAC) || defined(Q_OS_LINUX)
    return io::unmount(this->devicePath, true);
#endif
#if defined (Q_OS_WIN) || defined(Q_OS_WIN32)
    /* Poor mans solution: trash MBR, which we do during write later */
    return true;
#endif
}

void ExtractProgress::doExtraction()
{
    status = EXTRACTING_STATUS;
    utils::writeLog("Extracting " + deviceImage);

    QThread* thread = new QThread;
    ExtractWorker *worker = new ExtractWorker(deviceImage, QString(deviceImage).remove(".gz"));

    worker->moveToThread(thread);
    connect(worker, SIGNAL(error()), this, SLOT(extractError()));
    connect(thread, SIGNAL(started()), worker, SLOT(process()));
    connect(worker, SIGNAL(progressUpdate(unsigned)), this, SLOT(setProgress(unsigned)));
    connect(worker, SIGNAL(finished()), thread, SLOT(quit()));
    connect(worker, SIGNAL(finished()), worker, SLOT(deleteLater()));
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    connect(thread, SIGNAL(finished()), this, SLOT(finished()));
    thread->start();
}

void ExtractProgress::extractError()
{
    ui->extractProgressBar->setValue(0);
    /* need to make sure progressBar is not idling */
    ui->extractProgressBar->setMaximum(100);
    ui->extractDetailsLabel->setText(tr("An error occured extracting the archive!") + tr("\nPlease consult the logfile."));
}

void ExtractProgress::writeError()
{
    ui->extractProgressBar->setValue(0);
    /* need to make sure progressBar is not idling */
    ui->extractProgressBar->setMaximum(100);
    ui->extractDetailsLabel->setText(tr("An error occured while writing the image!") + tr("\nPlease consult the logfile."));
}

void ExtractProgress::setProgress(unsigned written)
{
    if(status == EXTRACTING_STATUS){
        ui->extractProgressBar->setValue(written);
        ui->extractDetailsLabel_2->setText(tr("Extracting") + " " + QString::number(written / 1024 / 1024) + "MB");
    }
    if(status == WRITING_STATUS){
        ui->extractProgressBar->setValue(written);
        ui->extractDetailsLabel_2->setText(tr("Written") + " " + QString::number(written / 1024 / 1024) + "MB");
    }
}

void ExtractProgress::finished()
{
    utils::writeLog("Finished extraction. Going to write image");
    ui->extractDetailsLabel_2->setText("");
    writeImageToDisk();
}

void ExtractProgress::writeFinished()
{
    utils::writeLog("Image successfully written to device");
    utils::writeLog("Deleting the uncompressed image to save space");
    QFile uncompressedFile(QString(deviceImage).remove(".gz"));
    if (uncompressedFile.exists())
        uncompressedFile.remove();

    emit(finishedExtraction());
}

void ExtractProgress::setFlushing()
{
    ui->extractProgressBar->setMinimum(0);
    ui->extractProgressBar->setMaximum(0);
    ui->extractDetailsLabel_2->setText("");
    ui->extractDetailsLabel->setText(tr("Finalising") + " " + this->devicePath + tr("\n(This might take a minute or two!)"));
}

ExtractProgress::~ExtractProgress()
{
    delete ui;
}
