#include "mainwindow.h"
#include "utils.h"
#include "logger.h"
#include <QFontDatabase>
#include <QGraphicsOpacityEffect>
#include "sys/mount.h"
#include <QDesktopWidget>
#include <QApplication>
#ifdef Q_WS_QWS
#include <QWSServer>
#endif
#include <QFile>
#include "targetlist.h"
#include "target.h"
#include <QTranslator>
#include <QThread>
#include "extractworker.h"
#include <QStyle>
#include <QFontInfo>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent) :
    QWidget(parent), device(NULL), preseed(NULL)

{
    /* Set up logging */
    logger = new Logger();
    logger->addLine("Starting OSMC installer");

    /* UI set up */
    #ifdef Q_WS_QWS
    QWSServer *server = QWSServer::instance();
    if(server)
    {
        server->setCursorVisible(false);
        server->setBackground(QBrush(Qt::black));
        this->setWindowFlags(Qt::Tool|Qt::CustomizeWindowHint);
    }
    #endif

    setupSizeAndBackground();

    layout = new QVBoxLayout(this);
    layout->addSpacerItem(new QSpacerItem(appWidth, (int) (appHeight*0.7)));

    setupStatusLabel();
    setupProgressBar();
    setupCopyrightLabel();

    layout->addSpacerItem(new QSpacerItem(appWidth, (int) (appHeight*0.1)));
    setLayout(layout);
    /* Populate target list map */
    targetList = new TargetList();
    utils = new Utils(logger);
}

void MainWindow::setupStatusLabel()
{
    statusLabel = new QLabel("Just a moment");
    statusLabel->setStyleSheet("border-top-right-radius: 20px; border-top-left-radius: 20px; background-color: rgba(0,0,0,70); color:rgb(240, 240, 240);");
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setMaximumWidth(appWidth*0.93);
    statusLabel->setMinimumWidth(appWidth*0.93);
    statusLabel->setMaximumHeight(appHeight*0.13);
    statusLabel->setMinimumHeight(appWidth*0.13);
    layout->addWidget(statusLabel, Qt::AlignCenter);
    statusLabel->setFont(getFont("Status Label", statusLabel, 23));
}

void MainWindow::setupProgressBar()
{
    progressBar = new QProgressBar();
    progressBar->setStyleSheet("border-radius: 0px; background-color: rgba(0,0,0,70); color:rgb(240, 240, 240);");
    progressBar->setAlignment(Qt::AlignCenter);
    progressBar->setMaximumWidth(appWidth*0.93);
    progressBar->setMinimumWidth(appWidth*0.93);
    progressBar->setMaximumHeight(appHeight*0.10);
    progressBar->setMinimumHeight(appWidth*0.10);
    layout->addWidget(progressBar, Qt::AlignCenter);
    progressBar->setFont(getFont("42", progressBar, 15));
}

void MainWindow::setupCopyrightLabel()
{
    copyrightLabel = new QLabel("(c) 2014 OSMC");
    copyrightLabel->setStyleSheet("border-bottom-right-radius: 20px; border-bottom-left-radius: 20px; background-color: rgba(0,0,0,70); color:rgb(240, 240, 240);");
    copyrightLabel->setAlignment(Qt::AlignCenter);
    copyrightLabel->setMaximumWidth(appWidth*0.93);
    copyrightLabel->setMinimumWidth(appWidth*0.93);
    copyrightLabel->setMaximumHeight(appHeight*0.04);
    copyrightLabel->setMinimumHeight(appWidth*0.04);
    layout->addWidget(copyrightLabel, Qt::AlignCenter);
    copyrightLabel->setFont(getFont("(c) 2014 OSMC", copyrightLabel, 10));
}

void MainWindow::setupSizeAndBackground()
{
    size = QApplication::desktop()->size();
    appWidth = size.width();
    appHeight = size.height();

    setFixedSize(appWidth, appHeight);

    QPalette palette;
    QImage image(":/assets/resources/install.png");
    image = image.scaled(QSize(appWidth, appHeight));
    palette.setBrush(this->backgroundRole(), QBrush(image));

    this->setPalette(palette);
    //setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, size, qApp->desktop()->availableGeometry()));
}

/*
 * stolen from http://www.qtforum.org/article/32092/dynamic-font-size-in-qlabel-based-on-text-length.html
 */
QFont MainWindow::getFont(QString refString, QWidget* element, int refPointSize) {
    bool fits = false;
    QFont* font = new QFont("Source Sans Pro", refPointSize);
    element->repaint();
    int refWidth = element->width();
    int refHeight = element->height();
    while (!fits)
        {
            QFontMetrics fm( *font );
            QRect bound = fm.boundingRect(refString);
            if (bound.width() <= refWidth && bound.height() <= refHeight)
                fits = true;
            else
                font->setPointSize(font->pointSize() - 1);
        }

    QFontInfo fi(*font);
    qDebug() << "determined font for " << refString << fi.family() << fi.pointSize();
    return *font;
}

void MainWindow::install()
{
    qApp->processEvents();
    /* Find out what device we are running on */
    logger->addLine("Detecting device we are running on");
    device = targetList->getTarget(utils->getOSMCDev());
    if (device == NULL)
    {
        haltInstall("unsupported device"); /* No tr here as not got lang yet */
        return;
    }
    /* Mount the BOOT filesystem */
    logger->addLine("Mounting boot filesystem");
    if (! utils->mountPartition(device, MNT_BOOT))
    {
        haltInstall("could not mount bootfs");
        return;
    }
    /* Sanity check: need filesystem.tar.xz */
    QFile fileSystem(QString(MNT_BOOT) + "/filesystem.tar.xz");
    if (! fileSystem.exists())
    {
        haltInstall("no filesystem found");
        return;
    }
    /* Load in preseeded values */
    preseed = new PreseedParser();
    if (preseed->isLoaded())
    {
        logger->addLine("Preseed file found, will attempt to parse");
        /* Locales */
        locale = preseed->getStringValue("globe/locale");
        if (! locale.isEmpty())
        {
            logger->addLine("Found a definition for globalisation: " + locale);
            QTranslator translator;
            if (translator.load(qApp->applicationDirPath() + "/osmc_" + locale + ".qm"))
            {
                logger->addLine("Translation loaded successfully!");
                qApp->installTranslator(&translator);                
            }
            else
                logger->addLine("Could not load translation");
        }
        /* Install target */
        installTarget = preseed->getStringValue("target/storage");
        if (! installTarget.isEmpty())
        {
            logger->addLine("Found a definition for storage: " + installTarget);
            if (installTarget == "nfs")
            {
                QString nfsPath = preseed->getStringValue("target/storagePath");
                if (! nfsPath.isEmpty())
                {
                    device->setRoot(nfsPath);
                    useNFS = true;
                }
            }
            if (installTarget == "usb")
            {
                /* Behaviour for handling USB installs */
                if (utils->getOSMCDev() == "rbp") { device->setRoot("/dev/sda1"); }
                statusLabel->setText(tr("USB install: 60 seconds to remove device before data loss"));
                qApp->processEvents();
                system("/bin/sleep 60");
            }
        }
        /* Bring up network if using NFS */
        if (useNFS)
        {
            logger->addLine("NFS installation chosen, must bring up network");
            nw = new Network();
            nw->setIP(preseed->getStringValue("network/ip"));
            nw->setMask(preseed->getStringValue("network/mask"));
            nw->setGW(preseed->getStringValue("network/gw"));
            nw->setDNS1(preseed->getStringValue("network/dns1"));
            nw->setDNS2(preseed->getStringValue("network/dns2"));
            if (! nw->isDefined())
            {
                logger->addLine("Either network preseed definition incomplete, or user wants DHCP");
                nw->setAuto();
                logger->addLine("Attempting to bring up eth0");
                statusLabel->setText(tr("Configuring Network"));
                nw->bringUp();
            }
        }
    }
    else
    {
        logger->addLine("No preseed file was found");
    }
    /* If !nfs, create necessary partitions */
    statusLabel->setText(tr("Partitioning device"));
    if (! useNFS)
    {
        logger->addLine("Creating root partition");
        statusLabel->setText(tr("Formatting device"));
        QString rootBase = device->getRoot();
        if (rootBase.contains("mmcblk"))
            rootBase.chop(2);
        else
            rootBase.chop(1);
        logger->addLine("From a root partition of " + device->getRoot() + ", I have deduced a base device of " + rootBase);
        if (device->hasRootChanged())
        {
            logger->addLine("Must mklabel as root fs is on another device");
            utils->mklabel(rootBase, false);
            utils->mkpart(rootBase, "ext4", "4096s", "100%");
            utils->fmtpart(device->getRoot(), "ext4");
        }
        else
        {
            int size = utils->getPartSize(rootBase, (device->getBootFS() == "vfat" ? "fat32" : "ext4"));
            if (size == -1)
            {
                logger->addLine("Issue getting size of device");
                haltInstall(tr("cannot work out partition size"));
                return;
            }
            logger->addLine("Determined " + QString::number(size) + " MB as end of first partition");
            utils->mkpart(rootBase, "ext4", QString::number(size + 2) + "M", "100%");
            utils->fmtpart(device->getRoot(), "ext4");
        }
    }
    /* Mount root filesystem */
    if (useNFS)
        bc = new BootloaderConfig(device, nw, utils, logger);
    else
        bc = new BootloaderConfig(device, NULL, utils, logger);
    logger->addLine("Mounting root");
    if ( ! utils->mountPartition(device, MNT_ROOT))
    {
        logger->addLine("Error occured trying to mount root of " + device->getRoot());
        haltInstall(tr("can't mount root"));
        return;
    }
   /* Extract root filesystem */
   statusLabel->setText(tr("Installing files"));
   logger->addLine("Extracting files to root filesystem");
   progressBar->setMinimum(0);
   progressBar->setMaximum(100);
   QThread* thread = new QThread(this);
   ExtractWorker *worker = new ExtractWorker(fileSystem.fileName(), MNT_ROOT, logger);
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
    progressBar->setMinimum(0);
    progressBar->setMaximum(4);
    statusLabel->setText(tr("Configuring bootloader"));
    logger->addLine("Configuring bootloader: moving /boot to appropriate boot partition");
    bc->copyBootFiles();
    progressBar->setValue(1);
    logger->addLine("Configuring boot cmdline");
    bc->configureEnvironment();
    progressBar->setValue(2);
    logger->addLine("Configuring /etc/fstab");
    bc->configureMounts();
    progressBar->setValue(3);
    /* Dump the log */
    logger->addLine("Successful installation. Dumping log and rebooting system");
    dumpLog();
    progressBar->setValue(4);
    /* Reboot */
    statusLabel->setText(tr("Installation successful! Rebooting..."));
    qApp->processEvents(); /* Force GUI update */
    utils->rebootSystem();
}

void MainWindow::haltInstall(QString errorMsg)
{
    logger->addLine("Halting Install. Error message was: " + errorMsg);
    progressBar->setMaximum(100);
    progressBar->setValue(0);
    statusLabel->setText(tr("Install failed: ") + errorMsg);
    dumpLog();
}

void MainWindow::dumpLog()
{
    QFile logFile("/mnt/boot/install.log");
    utils->writeToFile(logFile, logger->getLog(), false);
}

void MainWindow::finished()
{
    logger->addLine("Extraction of root filesystem completed");
    logger->addLine("Configuring bootloader");
    setupBootLoader();
}

void MainWindow::setProgress(unsigned value)
{
    progressBar->setValue(value);
}

MainWindow::~MainWindow()
{
    delete nw;
    delete bc;
    delete device;
    delete logger;
    delete this->targetList;
    delete preseed;
}
