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
#include <QDesktopWidget>
#include <QApplication>

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
        this->setWindowFlags(Qt::Tool|Qt::CustomizeWindowHint);
    #endif
    this->setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, this->size(), qApp->desktop()->availableGeometry()));
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
    if (mountRW)
        logger->addLine("Attempting to mount partition: " + bootPart + " " + "as RW");
    else
        logger->addLine("Attempting to mount partition: " + bootPart + " " + "as RO");
    if (bootFs == "vfat")
        mountStatus = mount(bootPart.toLocal8Bit(), "/mnt", "vfat", 0, "");
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
    QFile fsTarball("/mnt/filesystem.tar.xz");
    if (fsTarball.exists())
        logger->addLine("Filesystem tarball exists!");
    else
    {
        logger->addLine("No filesystem tarball was found");
        haltInstall("No filesystem found!"); /* No tr here as not got lang yet */
        return;
    }
    /* Check for a preseeding file */
    QStringList preseedStringList;
    logger->addLine("Checking for a preseed file in /mnt");
    QFile preseedFile("/mnt/preseed.cfg");

    if (preseedFile.exists())
    {
        if (!preseedFile.open(QIODevice::ReadOnly | QIODevice::Text))
            haltInstall("Preseed file exists but could not be opened. Erros is " + preseedFile.errorString());

        logger->addLine("Preseed file was found");
        QTextStream preseedStream(&preseedFile);
        QString preseedString = preseedStream.readAll();
        preseedFile.close();
        preseedStringList = preseedString.split(QRegExp("[\r\n]"),QString::SkipEmptyParts);
    }
    else
        logger->addLine("Preseed file does not exist");
    /* Check preseeding for our install type */

    for (int i = 0; i < preseedStringList.count(); i++)
    {
        QString pString = preseedStringList.at(i);
        QStringList pStringSplit = pString.split(" ");
        QString valueString;
        bool valueisBool;
        valueisBool = (pStringSplit.at(2) == "boolean") ? true : false;
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

            continue;
        }
        if (pString.contains("target/storagePath"))
        {
            logger->addLine("Found storage path definition: " + valueString);
            storagePathString = valueString;
            continue;
        }
        if (pString.contains("network/ip"))
        {
            logger->addLine("Found IP address entry");
            ip = valueString;
            continue;
        }
        if (pString.contains("network/mask"))
        {
            logger->addLine("Found netmask address entry");
            subnet = valueString;
            continue;
        }
        if (pString.contains("network/dns1"))
        {
            logger->addLine("Found dns1 address entry");
            dns1 = valueString;
            continue;
        }
        if (pString.contains("network/dns2"))
        {
            logger->addLine("Found dns2 address entry");
            dns2 = valueString;
            continue;
        }
        if (pString.contains("network/gw"))
        {
            logger->addLine("Found gateway address entry");
            gw = valueString;
            continue;
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
        QFile interfacesFile("/etc/network/interfaces");
        interfacesFile.open(QIODevice::WriteOnly | QIODevice::Text);
        QTextStream interfacesStream(&interfacesFile);
        for (int i = 0; i < interfacesStringList->count(); i++)
        {
            interfacesStream << interfacesStringList->at(i) + "\n";
        }
        interfacesFile.close();
        ui->statusLabel->setText(tr("Starting network"));
        logger->addLine("Bringing up eth0");
        QProcess ethProcess;
        ethProcess.start("ifup eth0");
        ethProcess.waitForFinished();
    }
    /* Create partitions if necessary (i.e. not NFS) */
    if (storageTypeString != "nfs")
    {
        ui->statusLabel->setText(tr("Partitioning device"));
        logger->addLine("We are not installing to an NFS share. Partitioning required");
        if (storageTypeString == "usb")
        {
            logger->addLine("USB install chosen. Will do some sanity checking");
            /* ToDo: 1) check for two USB devices
             * 2) display a warning for 60 secs that drive will be erased
             */
            if (dev == "rbp")
            {
                logger->addLine("Raspberry Pi with USB install: will create msdos partition layout on /dev/sda with one ext4 partition");
                utils::mklabel("/dev/sda", false);
                utils::mkpart("/dev/sda", "ext4", "4096s", "100%");
                utils::fmtpart("/dev/sda1", "ext4");
            }
        }
        if (storageTypeString == "sd")
        {
            logger->addLine("SD card install chosen. Will do some sanity checking");
            /* ToDo: check the size of the SD card, needs to be 2GB */
            /* Already have a label, just mkpart */
            if (dev == "rbp")
            {
                logger->addLine("Raspberry Pi with SD card install: will make second partition as ext4");
                if (! utils::mkpart("/dev/mmcblk0p1", "ext4", "258 M", "100%"))
                {
                    logger->addLine("mkpart failed!");
                    haltInstall(tr("Error creating partition"));
                }
                else if (! utils::fmtpart("/dev/mmcblk0p2", "ext4"))
                {
                    logger->addLine("formatting /dev/mmcblk0p2 failed");
                    haltInstall(tr("Error formatting partition"));
                }
            }
        }
    }
    /* Mount our root filesystem */
    system("mkdir /rfs");
    int successMount;
    if (dev == "rbp" && storageTypeString == "sd")
        successMount = mount("/dev/mmcblk0p2", "/rfs", "ext4", 0, "");
    if (dev == "rbp" && storageTypeString == "usb");
        successMount = mount("/dev/sda1", "/rfs", "ext4", 0, "");
    if (successMount != 0)
    {
        logger->addLine("Mounting root filesystem failed!");
        haltInstall(tr("Mounting root filesystem failed!"));
    }
    /* Extract root filesystem to /rfs */
    ui->statusLabel->setText(tr("Installing files"));
    logger->addLine("Extracting files");
    ui->statusProgressBar->setMinimum(0);
    ui->statusProgressBar->setMaximum(100);
    QThread* thread = new QThread;
    ExtractWorker *worker = new ExtractWorker(fsTarball.fileName(), "/rfs");
    worker->moveToThread(thread);
    connect(thread, SIGNAL(started()), worker, SLOT(extract()));
    connect(worker, SIGNAL(progressUpdate(unsigned)), this, SLOT(setProgress(unsigned)));
    connect(worker, SIGNAL(error(QString)), this, SLOT(haltInstall(QString)));
    connect(worker, SIGNAL(finished()), thread, SLOT(quit()));
    connect(worker, SIGNAL(finished()), worker, SLOT(deleteLater()));
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    connect(thread, SIGNAL(finished()), this, SLOT(finished()));
    thread->start();
}

void MainWindow::setupBootLoader()
{
    /* Set up the boot loader */
    ui->statusLabel->setText(tr("Configuring bootloader"));
    logger->addLine("Configuring bootloader: moving /boot to appropriate boot partition");
    system("mv -ar /rfs/boot/* /mnt");
    if (dev == "rbp")
    {
        logger->addLine("Configuring cmdline.txt");
        QFile cmdlineFile("/mnt/cmdline.txt");
        QStringList cmdlineStringList;
        if (storageTypeString == "sd")
            cmdlineStringList << "root=/dev/mmcblk0p2 rootfstype=ext4 rootwait quiet";
        if (storageTypeString == "usb")
            cmdlineStringList << "root=/dev/sda1 rootfstype=ext4 rootwait quiet";
        utils::writeToFile(cmdlineFile, cmdlineStringList, false);
    }
    /* Set up /etc/fstab */
    logger->addLine("Configuring /etc/fstab");
    QFile fstabFile("/rfs/etc/fstab");
    QStringList fstabStringList;
    if (dev == "rbp")
    {
        fstabStringList.append("/dev/mmcblk0p1  /boot           vfat    defaults,noatime         0       0");
        if (storageTypeString == "sd")
            fstabStringList.append("/dev/mmcblk0p2  /               ext4    defaults,noatime 0       0");
        if (storageTypeString == "usb")
            fstabStringList.append("/dev/sda1  /               ext4    defaults,noatime 0       0");
    }
    utils::writeToFile(fstabFile, fstabStringList, true);
    /* Dump the log */
    dumpLog();
    /* Reboot */
    utils::rebootSystem();
}

void MainWindow::haltInstall(QString errorMsg)
{
    logger->addLine("Halting Install. Error message was: " + errorMsg);
    ui->statusProgressBar->setMaximum(100);
    ui->statusProgressBar->setValue(0);
    ui->statusLabel->setText(tr("Install failed: ") + errorMsg);
    dumpLog();
}

void MainWindow::dumpLog()
{
    /* Attempts to write to /mnt; may not *actually* be mounted */
    /* Could check etc/mtab but it's irrelevant if it is mounted or not */
    QFile logFile("/mnt/install.log");
    utils::writeToFile(logFile, logger->getLog(), false);
}

void MainWindow::finished()
{
    logger->addLine("Extraction finished.");
    logger->addLine("going to setup the bootloader");
    setupBootLoader();
}

void MainWindow::setProgress(unsigned value)
{
    ui->statusProgressBar->setValue(value);
}

MainWindow::~MainWindow()
{
    delete ui;
}
