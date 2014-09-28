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
    this->deviceImage = QString(deviceImage);

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
    ui->extractDetailsLabel->setText("Unmounting " + this->devicePath);
    utils::writeLog("Requesting confirmation from user");
    const char* message = ("Do you want to image the device " + this->devicePath + "? OSMC is not responsible for loss of personal data").toUtf8().constData();
    if (utils::promptYesNo(this, tr("Are you sure"), tr(message)))
    {
        utils::writeLog("User confirmed");
        bool unmountSuccess = unmountDisk();

        if (unmountSuccess == false)
        {
            utils::displayError("Unmount failed!", "Could not unmount device " + devicePath + ". Check the log for error messages. Will have to quit now.", true);
            QApplication::quit();
            return;
        }

        /*
         * Ok, everything seems fine now.
         * Should we do a double check if the device is really unmounted?
         * Well, I'm paranoid.
         */

        ui->extractDetailsLabel->setText("Writing image to " + this->devicePath);

#ifdef Q_OS_MAC
        /* At the moment, We can't provide a real progress bar on OSX, so set up a busy bar here */
        ui->extractProgressBar->setMaximum(0);
        ui->extractProgressBar->setMinimum(0);
#endif

        QThread* thread = new QThread;
        WriteImageWorker *worker = new WriteImageWorker(this->deviceImage, this->devicePath);

        worker->moveToThread(thread);
        connect(worker, SIGNAL(error()), this, SLOT(writeError()));
        connect(thread, SIGNAL(started()), worker, SLOT(process()));
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
    return io::unmountDisk(this->devicePath);
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
    ui->extractDetailsLabel->setText(tr("An error occured extracting the archive!"));
}

void ExtractProgress::writeError()
{
    ui->extractProgressBar->setValue(0);
    ui->extractDetailsLabel->setText(tr("An error occured while writing the image!"));
}

void ExtractProgress::setProgress(unsigned written)
{
    if(status = EXTRACTING_STATUS){
        ui->extractProgressBar->setValue(written);
        ui->extractDetailsLabel->setText("Unzipping " + QString::number(written / 1024 / 1024) + "MB");
    }
    if(status = WRITING_STATUS){
        ui->extractProgressBar->setValue(written);
        ui->extractDetailsLabel->setText("Written " + QString::number(written / 1024 / 1024) + "MB");
    }
}

/*!
 * \brief ExtractProgress::finished
 * Our worker has signalled finished. Now we can write the image.
 *
 */
void ExtractProgress::finished()
{
    utils::writeLog("Finished extraction. Going to write image");
    writeImageToDisk();
}

void ExtractProgress::writeFinished()
{
    /* HOOOOORAY! */
    ui->extractDetailsLabel->setText("Finished");
    ui->extractProgressBar->hide();

    utils::writeLog("Finished extraction. Going to write image");
    utils::writeLog("HOOOOOORAY!");
}

ExtractProgress::~ExtractProgress()
{
    delete ui;
}
