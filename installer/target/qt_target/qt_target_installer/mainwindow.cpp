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
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QThread>
#include <QTranslator>
#include <QWSServer>
#include "extractworker.h"

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
    ui->copyrightLabel->setGraphicsEffect(ope);
    ui->statusProgressBar->setGraphicsEffect(ope);
}

void MainWindow::install()
{
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
    if (mountRW)
        logger->addLine("Attempting to mount partition: " + bootPart + " " + "as RW");
    else
        logger->addLine("Attempting to mount partition: " + bootPart + " " + "as RO");
    if (bootFs == "vfat")
        mountStatus = mount(bootPart.toLocal8Bit(), "/mnt", "vfat", 1, "");
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
    /* Check we have a filesystem for target */
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
/*
    QString mntPath = "/Users/srm/filesysTest/out";

    ui->statusProgressBar->setMinimum(0);
    ui->statusProgressBar->setMaximum(100);
    QThread* thread = new QThread;
    ExtractWorker *worker = new ExtractWorker(fsTarball.fileName(), mntPath);
    worker->moveToThread(thread);
    connect(thread, SIGNAL(started()), worker, SLOT(extract()));
    connect(worker, SIGNAL(progressUpdate(unsigned)), this, SLOT(setProgress(unsigned)));
    connect(worker, SIGNAL(error(QString)), this, SLOT(haltInstall(QString)));
    connect(worker, SIGNAL(finished()), thread, SLOT(quit()));
    connect(worker, SIGNAL(finished()), worker, SLOT(deleteLater()));
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    connect(thread, SIGNAL(finished()), this, SLOT(finished()));

    logger->addLine("Starting extraction of " + fsTarball.fileName() + " to " + mntPath);

    thread->start();*/

    #endif
}

void MainWindow::preseed()
{
    /* Check for a preseeding file */
    QStringList preseedStringList;
    #ifdef Q_WS_QWS
    logger->addLine("Checking for a preseed file in /mnt");
    QFile preseedFile("/mnt/preseed.cfg");
    if (preseedFile.exists())
    {
        logger->addLine("Preseed file was found");
        QTextStream preseedStream(&preseedFile);
        QString preseedString;
        while (!preseedStream.atEnd())
        {
            preseedString = preseedStream.readAll();
        }
         preseedStringList = preseedString.split(QRegExp("[\r\n]"),QString::SkipEmptyParts);
    }
    else
        logger->addLine("Preseed file does not exist");
    #else
    logger->addLine("No preseed file as we are in Qt Creator, faking some preseeding");
    //preseedStringList.append("...");
    #endif
    /* Check preseeding for our install type */
    bool useNetwork = false;
    bool useNFS = false;
    QString storageTypeString;
    QString storagePathString;
    bool useDHCP = true;
    QString ip;
    QString subnet;
    QString gw;
    QString dns1;
    QString dns2;
    for (int i = 0; i < preseedStringList.count(); i++)
    {
        QString pString = preseedStringList.at(i);
        QStringList pStringSplit = pString.split(" ");
        QString valueString;
        bool valueisBool;
        valueisBool = (pStringSplit.at(2) == "bool") ? true : false;
        valueString = pStringSplit.at(3);
        if (pString.contains("globe/locale"))
        {
            logger->addLine("Found a definition for globalisation: " + valueString);
            QTranslator translator;
            if (translator.load(qApp->applicationDirPath() + "/osmc_" + valueString + ".qm"))
            {
                logger->addLine("Translation loaded successfully!");
                qApp->installTranslator(&translator);
                ui->retranslateUi(this);
            }
            else
                logger->addLine("Could not load translation");
        }
        if (pString.contains("target/storage"))
        {
            logger->addLine("Found a definition for storage: " + valueString);
            storageTypeString = valueString;
            if (valueString == "nfs")
                useNetwork = true;
        }
        if (pString.contains("target/storagePath"))
        {
            logger->addLine("Found storage path definition: " + valueString);
            storagePathString = valueString;
        }
        if (pString.contains("network/ip"))
        {
            logger->addLine("Found IP address entry");
            ip = valueString;
        }
        if (pString.contains("network/mask"))
        {
            logger->addLine("Found netmask address entry");
            subnet = valueString;
        }
        if (pString.contains("network/dns1"))
        {
            logger->addLine("Found dns1 address entry");
            dns1 = valueString;
        }
        if (pString.contains("network/dns2"))
        {
            logger->addLine("Found dns2 address entry");
            dns2 = valueString;
        }
        if (pString.contains("network/gw"))
        {
            logger->addLine("Found gateway address entry");
            gw = valueString;
        }
    }

    if (useNetwork)
    {
        QStringList *interfacesStringList = new QStringList();
        interfacesStringList->append(QString("auto eth0"));
        if (! ip.isEmpty() && ! subnet.isEmpty() && ! gw.isEmpty() && dns1.isEmpty() && dns2.isEmpty())
        {
            logger->addLine("All entries for manual IP configuration defined");
            interfacesStringList->append("iface eth0 inet static");
            interfacesStringList->append("\t address " + ip);
            interfacesStringList->append("\t netmask " + subnet);
            interfacesStringList->append("\t gateway " + gw);
            QFile nameserversFile("/etc/resolv.conf");
            nameserversFile.open(QIODevice::WriteOnly, QIODevice::Text);
            QTextStream nameserversTextStream(&nameserversFile);
            nameserversTextStream << "nameserver " + dns1;
            nameserversTextStream << "nameserver " + dns2;
            nameserversFile.close();
        }
        else
        {
            logger->addLine("No entries defined for manual IP, will use DHCP");
            interfacesStringList->append(QString("iface eth0 inet dhcp"));
        }
        logger->addLine("Going to write the following to /etc/network/interfaces");
        for (int i = 0; i < interfacesStringList->count(); i++)
        {
            logger->addLine(interfacesStringList->at(i));
        }
        #ifndef Q_WS_QWS
        QFile interfacesFile("/etc/network/interfaces");
        interfacesFile.open(QIODevice::WriteOnly | QIODevice::Text);
        QTextStream interfacesStream(&interfacesFile);
        for (int i = 0; i < interfacesStringList->count(); i++)
        {
            interfacesStream << interfacesStringList->at(i);
        }
        interfacesFile.close();
        #endif
        #ifndef Q_WS_QWS
        ui->statusLabel->setText(tr("Starting network"));
        logger->addLine("Bringing up eth0");
        QProcess ethProcess;
        ethProcess.start("ifup");
        ethProcess.waitForFinished();
        #endif
        /* We could add check here: like pinging gateway, but we know soon enough when we try mount */
    }
}


void MainWindow::haltInstall(QString errorMsg)
{
    logger->addLine("Halting Install. Error message was: " + errorMsg);
    ui->statusProgressBar->setMaximum(100);
    ui->statusProgressBar->setValue(0);
    ui->statusLabel->setText(tr("Install failed: ") + errorMsg);
    /* Attempts to write to /mnt; may not *actually* be mounted */
    /* Could check etc/mtab but it's irrelevant if it is mounted or not */
    #ifndef Q_WS_QWS
    QFile logFile("/mnt/install.log");
    logFile.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream logStream(&logFile);
    QStringList *logStringList = logger->getLog();
    for (int i = 0; i < logStringList->count(); i++)
    {
        logStream << logStringList->at(i);
    }
    logFile.close();
    #endif
}

void MainWindow::finished()
{
    logger->addLine("Extract finished.");
    preseed();
}

void MainWindow::setProgress(unsigned value)
{
    ui->statusProgressBar->setValue(value);
}

MainWindow::~MainWindow()
{
    delete ui;
}
