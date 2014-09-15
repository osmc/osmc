#include "extractprogress.h"
#include "ui_extractprogress.h"
#include <QString>
#include "utils.h"
#include "io.h"
#include "extractworker.h"
#include <QThread>

#define SET_BINARY_MODE(file)

ExtractProgress::ExtractProgress(QWidget *parent, QString devicePath, QString deviceImage):
    QWidget(parent),
    ui(new Ui::ExtractProgress)
{
    ui->setupUi(this);

    ui->extractProgressBar->setMaximum(io::getDecompressedSize(deviceImage));
    ui->extractProgressBar->setMinimum(0);

    bool extractSuccess = true; /* True as we only need to change if failed */
    if (deviceImage.contains(".gz"))
        extractSuccess = doExtraction(deviceImage);
    if (extractSuccess)
    {
        /* Write the image to the block device */
        //writeImageToDisc(devicePath, deviceImage);

    }
    /* Peform pre-seeding operations and final configuration */
}

bool ExtractProgress::writeImageToDisc(QString devicePath, QString deviceImage)
{
    #ifdef Q_OS_MAC
    io::writeImageOSX(devicePath, deviceImage);
    #endif
}


bool ExtractProgress::doExtraction(QString deviceImage)
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

ExtractProgress::~ExtractProgress()
{
    delete ui;
}
