#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "utils.h"
#include "logger.h"
#include <QFontDatabase>
#include <QGraphicsOpacityEffect>
#include "sys/mount.h"
#include <QDebug>
#include <QFile>
#include <QDir>
#ifndef Q_WS_QWS
#include "filesystem.h"
#endif

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setFixedSize(this->size());
    /* Set up logging */
    logger = new Logger();
    logger->addLine("Starting OSMC installer");
    /* UI set up */
    #ifdef Q_WS_QWS
    QWSServer *server = QWSServer::instance();
    if(server)
        server->setCursorVisible(false);
        server->setBackground(QBrush(Qt::black));
    #endif
    QFontDatabase fontDatabase;
    fontDatabase.addApplicationFont(":/assets/resources/SourceSansPro-Regular.ttf");
    QGraphicsOpacityEffect *ope = new QGraphicsOpacityEffect(this);
    ope->setOpacity(0.5);
    ui->statusLabel->setGraphicsEffect(ope);
    ui->statusProgressBar->setGraphicsEffect(ope);
    /* Find out what device we are running on */
    logger->addLine("Detecting device we are running on");
    dev = utils::getOSMCDev();
    if (! dev.isEmpty())
        logger->addLine(tr("Detected Device") + ": " + dev);
    else
    {
        logger->addLine("Could not determine device from /proc/cmdline");
        haltInstall("unsupported device"); /* No tr here as not got lang yet */
        return;
    }
    /* Find partition location */
    logger->addLine("Trying to find boot partition");
    QString bootPart;
    QString bootFs;
    bool mountRW = false; /* We can't mount ISOs RW */
    if (dev == "rbp")
    {
        bootPart = "/dev/mmcblk0p1";
        bootFs = "vfat";
        mountRW = true;
        logger->addLine("Boot partition should be at: " + bootPart);
    }
    if (bootPart.isEmpty())
    {
        logger->addLine("Could not resolve boot partition");
        haltInstall("cannot work out boot partition");
        return;
    }
    /* Mount the filesystem */
    int mountStatus;
    #ifdef Q_WS_QWS
    logger->addLine("Attempting to mount partition: " + bootPart + " " + "as " + (mountRW == 1) ? "RW" : "RO");
    if (bootFs = "vfat")
        mountStatus = mount(bootPart, "/mnt" "vfat", 1, "");
    #else
    logger->addLine("Fake mounting as we are in Qt Creator");
    mountStatus = 0;
    #endif
    if (mountStatus != 0)
    {
        logger->addLine("Mounting failed!");
        haltInstall("initial mount failed"); /* No tr here as not got lang yet */
        return;
    }
    else
        logger->addLine("Successfully mounted boot partition");
    logger->addLine("Checking for filesystem tarball");
    #ifdef Q_WS_QWS
    QFile fsTarball("/mnt/filesystem.tar.xz");
    if (fsTarball.exists())
        logger->addLine("Filesystem tarball exists!");
    else
    {
        logger->addLine("No filesystem tarball was found");
        haltInstall("No filesystem found!"); /* No tr here as not got lang yet */
        return;
    }
    #else
    QFile fsTarball(QDir::homePath().append("/filesystem.tar.xz"));
    logger->addLine("Faking filesystem in /mnt");
    /* Write a basic filesystem so we have something to play with */
    QByteArray fsByteArray;
    QDataStream fsDataStream(&fsByteArray, QIODevice::WriteOnly);
    fsDataStream.writeRawData((const char*) randomfile_tar_xz, randomfile_tar_xz_len);
    fsTarball.open(QIODevice::WriteOnly);
    fsTarball.write(fsByteArray);
    fsTarball.close();
    #endif
    /* Check for a preseeding file */

    /* Check for another language */

    /* Make sure we have a filesystem */

    /* Check preseeding for our install type */
}

void MainWindow::haltInstall(QString errorMsg)
{
    ui->statusProgressBar->setMaximum(100);
    ui->statusProgressBar->setValue(0);
    ui->statusLabel->setText(tr("Install failed: ") + errorMsg);
}

MainWindow::~MainWindow()
{
    delete ui;
}
