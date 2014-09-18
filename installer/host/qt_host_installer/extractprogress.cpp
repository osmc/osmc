#include "extractprogress.h"
#include "ui_extractprogress.h"
#include <QString>
#include "utils.h"
#include "io.h"
#include "extractworker.h"
#include <QThread>
#include <QMessageBox>
#include <QProcess>
#include <QWidget>
#include <QDebug>

#define SET_BINARY_MODE(file)

ExtractProgress::ExtractProgress(QWidget *parent, QString devicePath, QString deviceImage):
    QWidget(parent),
    ui(new Ui::ExtractProgress)
{
    ui->setupUi(this);

    ui->extractProgressBar->setMaximum(io::getDecompressedSize(deviceImage));
    ui->extractProgressBar->setMinimum(0);

    this->devicePath = QString(devicePath);
    this->deviceImage = QString(deviceImage);

}

void ExtractProgress::extract()
{
    if (deviceImage.endsWith(".gz"))
    {
        doExtraction();
    }
    else if (deviceImage.endsWith(".img"))
    {
        utils::writeLog("File claims to be already an 'img'. No need to extract.");
    }

}

void ExtractProgress::writeImageToDisc()
{
    bool reallyDoIt = userAllowsWrite();

    if (reallyDoIt == false)
    {
        utils::writeLog("User decided to abort before writing the image. Quitting ...");
        QApplication::quit();
    }
    else
    {
        utils::writeLog("User allowed to write " + deviceImage + " to " + devicePath);
        bool unmountSuccess = unmountDisk();

        #ifdef Q_OS_MAC
        //io::writeImageOSX(devicePath, deviceImage);
        #endif
    }
}


bool ExtractProgress::unmountDisk()
{
    #ifdef Q_OS_MAC
    //io::unmountDiskOSX(this->devicePath);
    #endif

        /* now check if we are really unmounted. maybe a bit clumsy - feel free to find a better solution */
}

bool ExtractProgress::userAllowsWrite()
{
    QString message = "Do you really want to write the image to\n"+devicePath+"?\n(If not, the Installer will quit.)";
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, tr("Please confirm"), message,QMessageBox::Yes|QMessageBox::No);

    return reply == QMessageBox::Yes;
}


void ExtractProgress::doExtraction()
{
    /* Based off http://www.zlib.net/zpipe.c */
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

void ExtractProgress::setProgress(unsigned written)
{
    ui->extractProgressBar->setValue(written);
    ui->extractDetailsLabel->setText("Unzipping " + QString::number(written / 1024 / 1024) + "MB");
}

/*!
 * \brief ExtractProgress::finished
 * Our worker has signalled finished. Now we can write the image.
 *
 */
void ExtractProgress::finished()
{
    utils::writeLog("Finished extraction. Going to write image");
    writeImageToDisc();
}

ExtractProgress::~ExtractProgress()
{
    delete ui;
}
