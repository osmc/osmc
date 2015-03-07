/*
 * (c) 2014-2015 Sam Nazarko
 * email@samnazarko.co.uk
*/
#include "mainwindow.h"
#include "ui_mainwindow.h"
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
#include <QTimer>

/* required definitions to construct the css for our pseudo-progressbar */
/* CSS_PROGRESS_IMAGE is used to reset potential other border-images, repeated on that component */
const QString MainWindow::CSS_PROGRESS_IMAGE = "border-image: foo;";
const QString MainWindow::CSS_PROGRESS_BORDER_STYLE = "border-style: outset;";
const QString MainWindow::CSS_PROGRESS_BORDER_WIDTH = "border-width: 2px;";
const QString MainWindow::CSS_PROGRESS_BORDER_RADIUS = "border-radius: 5px;";
/* this one is funny. need the border or the radius isn't displayed. so make it fully transparent */
const QString MainWindow::CSS_PROGRESS_BORDER_RGBA = "border-color: rgba(0,0,0,0);";
const QString MainWindow::CSS_PROGRESS_BACKGROUND_RGBA = "rgba(209,210,209, 255)";
const QString MainWindow::CSS_PROGRESS_BAR_RGBA = "rgba(24, 45, 81, 255)";

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow), device(NULL), preseed(NULL)

{
    ui->setupUi(this);

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
    this->setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, this->size(), qApp->desktop()->availableGeometry()));
    QFontDatabase fontDatabase;
    fontDatabase.addApplicationFont(":/assets/resources/SourceSansPro-Regular.ttf");

    /* Populate target list map */
    targetList = new TargetList();
    utils = new Utils(logger);
}

QFont MainWindow::getFont(QWidget* element, float ratio)
{
    QFont* font = new QFont("Source Sans Pro", 1);
    int refHeight = element->height();

    int fontSize = refHeight / ratio;
    font->setPointSize(fontSize);

    return *font;
}

void MainWindow::install()
{
    ui->statusLabel->setFont(getFont(ui->statusLabel, FONT_STATUSLABEL_RATIO));
    ui->statusProgressBar->setFont(getFont(ui->statusProgressBar, FONT_PROGRESSBAR_RATIO));

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
                ui->retranslateUi(this);
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
                if (utils->getOSMCDev() == "rbp1") { device->setRoot("/dev/sda1"); }
                if (utils->getOSMCDev() == "rbp2") { device->setRoot("/dev/sda1"); }
                for (int i = 0; i <= 60; i++)
                {
                    ui->statusLabel->setText(tr("USB install:") + " " + QString::number(60 - i) + " " + ("seconds to remove device before data loss"));
                    qApp->processEvents();
                    system("/bin/sleep 1");
                }
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
            }
            logger->addLine("Attempting to bring up eth0");
            ui->statusLabel->setText(tr("Configuring Network"));
            nw->bringUp();
        }
    }
    else
    {
        logger->addLine("No preseed file was found");
    }
    /* If !nfs, create necessary partitions */
    if (! useNFS)
    {
        logger->addLine("Creating root partition");
        ui->statusLabel->setText(tr("Formatting device"));
        qApp->processEvents(); /* Force GUI update */
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
    if (useNFS)
        system("rm -rf /mnt/root/*"); /* BusyBox tar does not like overwrites. Clear nfsroot first */
   /* Extract root filesystem */
   ui->statusLabel->setText(tr("Installing files"));
   logger->addLine("Extracting files to root filesystem");

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
    /* I have no idea why I can't give the value directly to SLOT(...) */
    unsigned val = 0;
    QTimer::singleShot(0, this, SLOT(val));
    val += 25;
    /* Set up the boot loader */
    ui->statusLabel->setText(tr("Configuring bootloader"));
    logger->addLine("Configuring bootloader: moving /boot to appropriate boot partition");
    bc->copyBootFiles();
    QTimer::singleShot(0, this, SLOT(val));
    val += 25;
    logger->addLine("Configuring boot cmdline");
    bc->configureEnvironment();
    QTimer::singleShot(0, this, SLOT(val));
    val += 25;
    logger->addLine("Configuring /etc/fstab");
    bc->configureMounts();
    QTimer::singleShot(0, this, SLOT(val));
    val += 25;
    /* Dump the log */
    logger->addLine("Successful installation. Dumping log and rebooting system");
    dumpLog();
    QTimer::singleShot(0, this, SLOT(val));

    /* Reboot */
    ui->statusLabel->setText(tr("OSMC installed successfully"));
    qApp->processEvents(); /* Force GUI update */
    utils->rebootSystem();
}

void MainWindow::haltInstall(QString errorMsg)
{
    logger->addLine("Halting Install. Error message was: " + errorMsg);
    setProgress(0);
    ui->statusLabel->setText(tr("Install failed: ") + errorMsg);
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
    QString styleSheet = "";

    styleSheet.append(CSS_PROGRESS_BORDER_RGBA)
            .append(CSS_PROGRESS_BORDER_RADIUS)
            .append(CSS_PROGRESS_BORDER_STYLE)
            .append(CSS_PROGRESS_BORDER_WIDTH)
            .append(CSS_PROGRESS_IMAGE)
            .append(getProgressbarGradient(value));

    ui->statusProgressBar->setStyleSheet(styleSheet);
}

QString MainWindow::getProgressbarGradient(unsigned value)
{
    float actualValue = value / 100.0;
    if (value == 100)
        actualValue = 0.999999;
    QString stopValue = QString::number(actualValue, 'g');

    /* required offset or the gradient will "flip" The more decimal places the sharper the cut between the two colors */
    QString stopValue2 = QString::number(actualValue + 0.000001, 'g');

    QString result = "background: qlineargradient(spread:pad, x1:0, y1:0, x2:1, y2:0, ";
    result.append("stop:0 ").append(CSS_PROGRESS_BAR_RGBA);
    result.append(", ");
    result.append("stop:").append(stopValue).append(" ").append(CSS_PROGRESS_BAR_RGBA);
    result.append(", ");
    result.append("stop:").append(stopValue2).append(" ").append(CSS_PROGRESS_BACKGROUND_RGBA);
    result.append(", ");
    result.append("stop:1 ").append(CSS_PROGRESS_BACKGROUND_RGBA).append(");");

    return result;
}

MainWindow::~MainWindow()
{
    delete ui;
    delete nw;
    delete bc;
    delete device;
    delete logger;
    delete this->targetList;
    delete preseed;
}
