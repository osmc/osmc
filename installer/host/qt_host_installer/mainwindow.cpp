/*
 * (c) 2014-2015 Sam Nazarko
 * email@samnazarko.co.uk
*/
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "langselection.h"
#include "updatenotification.h"
#include "versionselection.h"
#include "preseeddevice.h"
#include "networksetup.h"
#include "deviceselection.h"
#include "utils.h"
#include <QString>
#include <QTranslator>
#include "supporteddevice.h"
#include "advancednetworksetup.h"
#include "wifinetworksetup.h"
#include "networksettings.h"
#include <QList>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include "io.h"
#include "diskdevice.h"
#include "licenseagreement.h"
#include "downloadprogress.h"
#include "extractprogress.h"
#include "successdialog.h"
#include "preseeder.h"
#include <QMovie>
#if defined (Q_OS_MAC) || defined (Q_OS_LINUX)
    #include <sys/mount.h>
    #include <QDir>
#endif
#ifdef Q_OS_LINUX
#include <unistd.h>
#endif

#define WIDGET_START QPoint(10,110)

UpdateNotification *updater;
LangSelection *ls;
VersionSelection *vs;
PreseedDevice *ps;
NetworkSetup *ns;
DeviceSelection *ds;
AdvancedNetworkSetup *ans;
WiFiNetworkSetup *wss;
LicenseAgreement *la;
DownloadProgress *dp;
ExtractProgress *ep;
SuccessDialog *sd;

QTranslator translator;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    this->setFixedSize(this->size());
    ui->setupUi(this);
    ui->backButton->hide();
    connect(ui->backButton, SIGNAL(clicked()), this, SLOT(goBack()));

    /* Attempt auto translation */
    QString autolocale = QLocale::system().name();
    utils::writeLog("Detected locale as " + autolocale);
    translate(autolocale);
    /* Resolve a mirror URL */
    spinner = new QMovie(":/assets/resources/spinner.gif");
    ui->spinnerLabel->setMovie(spinner);
    spinner->start();
    this->mirrorURL = "http://download.osmc.tv";
    utils::writeLog("Resolving a mirror");
    accessManager = new QNetworkAccessManager(this);
    connect(accessManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));
    QNetworkRequest request(this->mirrorURL);
    accessManager->get(request);
    updater = new UpdateNotification(this);
    updater->hide();
    connect(updater, SIGNAL(hasUpdate()), this, SLOT(showUpdate()));
}

void MainWindow::rotateWidget(QWidget *oldWidget, QWidget *newWidget, bool enableBackbutton)
{
    this->ui->backButton->setEnabled(enableBackbutton);

    // make sure we update the state of the BackButton
    qApp->processEvents();

    if (this->widgetList.size() == 0)
    {
        this->widgetList.append(oldWidget);
    }

    this->widgetList.append(newWidget);

    oldWidget->hide();
    newWidget->move(WIDGET_START);
    newWidget->show();    
}

void MainWindow::replyFinished(QNetworkReply *reply)
{
    QVariant mirrorRedirectUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    this->mirrorURL = mirrorRedirectUrl.toString();
    utils::writeLog("Resolved mirror to " + this->mirrorURL);
    reply->deleteLater();
    ui->spinnerLabel->hide();
    /* Enumerating devices */
    QList<SupportedDevice *> devices = utils::buildDeviceList();
    ls = new LangSelection(this, devices);
    connect(ls, SIGNAL(languageSelected(QString, SupportedDevice)), this, SLOT(setLanguage(QString, SupportedDevice)));
    ls->move(WIDGET_START);
    ls->show();
    /* Check if an update exists */
    updater->isUpdateAvailable(mirrorURL);
}

void MainWindow::dismissUpdate() { rotateWidget(updater, ls); }

void MainWindow::showUpdate()
{
    connect(updater, SIGNAL(ignoreUpdate()), this, SLOT(dismissUpdate()));
    rotateWidget(ls, updater);
}

void MainWindow::setLanguage(QString language, SupportedDevice device)
{
        this->language = language;
        this->device = device;
        utils::writeLog("The user has selected " + this->language + " as their language");
        utils::writeLog("The user has selected " + this->device.getDeviceName() + " as their device");
        if (language != tr("English"))
        {
            translate(language);
        }
        else
        {
            /* Remove because we may have already done the deed */
            qApp->removeTranslator(&translator);
        }
        vs = new VersionSelection(this, this->device.getDeviceShortName(), this->mirrorURL);
        connect(vs, SIGNAL(versionSelected(bool, QUrl)), this, SLOT(setVersion(bool, QUrl)));
        ui->backButton->show();
        rotateWidget(ls, vs);
}

void MainWindow::goBack()
{

    this->widgetList.last()->deleteLater();
    this->widgetList.pop_back();
    this->widgetList.last()->move(WIDGET_START);
    this->widgetList.last()->show();

    if (this->widgetList.size() == 1)
    {
        ui->backButton->hide();
    }
}

void MainWindow::setVersion(bool isOnline, QUrl image)
{
    if (isOnline)
    {
        QString localImage = image.toString();
        localImage.replace("http://download.osmc.tv/", this->mirrorURL);
        localImage.remove("\n"); /* Handle line issues */
        localImage.remove("%0A"); /* other platforms might see this */
        image = QUrl(localImage);
        utils::writeLog("The user has selected an online image for " + this->device.getDeviceName() + " with build URL : " + image.toString());
        this->isOnline = true;
    }
    else
    {
        utils::writeLog("The user has selected a local image for " + this->device.getDeviceName() + " with file location: " + image.toString());
        this->isOnline = false;
    }
    this->image = image;
    /* We call the preseeder: even if we can't preseed, we use its callback to handle the rest of the application */
    ps = new PreseedDevice(this, this->device);
    connect(ps, SIGNAL(preseedSelected(int)), this, SLOT(setPreseed(int)));
    connect(ps, SIGNAL(preseedSelected(int, QString)), this, SLOT(setPreseed(int,QString)));
    rotateWidget(vs, ps);
}

void MainWindow::setPreseed(int installType)
{
    this->installType = installType;
    if (device.allowsPreseedingNetwork())
    {
        ns = new NetworkSetup(this, (this->installType == utils::INSTALL_NFS) ? false : true);
        connect(ns, SIGNAL(setNetworkOptionsInit(bool,bool)), this, SLOT(setNetworkInitial(bool,bool)));
        rotateWidget(ps, ns);
    }
    else
    {
        /* Straight to device selection */
        ds = new DeviceSelection(this);
        connect(ds, SIGNAL(DeviceSelected(DiskDevice*)), this, SLOT(selectDevice(DiskDevice*)));
        rotateWidget(ps, ds);
    }
}

void MainWindow::setPreseed(int installType, QString nfsPath)
{
    this->installType = utils::INSTALL_NFS;
    utils::writeLog("NFS installation to " + nfsPath + " selected");
    this->nfsPath = nfsPath;
    if (device.allowsPreseedingNetwork())
    {
        ns = new NetworkSetup(this, false);
        connect(ns, SIGNAL(setNetworkOptionsInit(bool,bool)), this, SLOT(setNetworkInitial(bool,bool)));
        rotateWidget(ps, ns);
    }
}

void MainWindow::setNetworkInitial(bool useWireless, bool advanced)
{
    nss = new NetworkSettings();
    if (advanced)
    {
        nss->setDHCP(false);
        if (!useWireless)
            nss->setWireless(false);
        else
            nss->setWireless(true);
        ans = new AdvancedNetworkSetup(this);
        connect(ans, SIGNAL(advancednetworkSelected(QString, QString, QString, QString, QString)), this, SLOT(setNetworkAdvanced(QString,QString,QString,QString,QString)));
        rotateWidget(ns, ans);
    }
    if (!advanced && useWireless)
    {
        nss->setDHCP(true);
        nss->setWireless(true);
        wss = new WiFiNetworkSetup(this);
        connect(wss, SIGNAL(wifiNetworkConfigured(QString,int,QString)), this, SLOT(setWiFiConfiguration(QString,int,QString)));
        rotateWidget(ns, wss);
    }
    if (!advanced && !useWireless)
    {
        nss->setDHCP(true);
        nss->setWireless(false);
        ds = new DeviceSelection(this);
        connect(ds, SIGNAL(DeviceSelected(DiskDevice*)), this, SLOT(selectDevice(DiskDevice*)));
        rotateWidget(ns, ds);
    }
}

void MainWindow::setNetworkAdvanced(QString ip, QString mask, QString gw, QString dns1, QString dns2)
{
    utils::writeLog("Setting custom non-DHCP networking settings");
    utils::writeLog("Set up network with IP: " + ip + " subnet mask of: " + mask + " gateway of: " + gw + " Primary DNS: " + dns1 + " Secondary DNS: " + dns2);
    nss->setIP(ip);
    nss->setMask(mask);
    nss->setGW(gw);
    nss->setDNS1(dns1);
    nss->setDNS2(dns2);
    if (nss->hasWireless())
    {
        wss = new WiFiNetworkSetup(this);
        connect(wss, SIGNAL(wifiNetworkConfigured(QString,int,QString)), this, SLOT(setWiFiConfiguration(QString,int,QString)));
        rotateWidget(ans, wss);
    }
    else
    {
        ds = new DeviceSelection(this);
        connect(ds, SIGNAL(DeviceSelected(DiskDevice*)), this, SLOT(selectDevice(DiskDevice*)));
        rotateWidget(ans, ds);
    }
}

void MainWindow::setWiFiConfiguration(QString ssid, int key_type, QString key_value)
{
    utils::writeLog("Wireless network configured with SSID " + ssid + " key value " + key_value);
    nss->setWirelessSSID(ssid);
    nss->setWirelessKeyType(key_type);
    /* No point if open */
    if (! (nss->getWirelessKeyType() == utils::WIRELESS_ENCRYPTION_NONE))
        nss->setWirelessKeyValue(key_value);
    ds = new DeviceSelection(this);
    connect(ds, SIGNAL(DeviceSelected(DiskDevice*)), this, SLOT(selectDevice(DiskDevice*)));
    rotateWidget(wss, ds);
}

void MainWindow::selectDevice(DiskDevice *nd)
{
    this->nd = nd;
    la = new LicenseAgreement(this);
    connect(la, SIGNAL(licenseAccepted()), this, SLOT(acceptLicense()));
    rotateWidget(ds, la);
    this->installDevicePath = nd->getDiskPath();
    this->installDeviceID = nd->getDiskID();
}

void MainWindow::acceptLicense()
{
    /* Move to Download widget, even if we aren't downloading */
    QUrl url;
    dp = new DownloadProgress(this);
    url = this->image;
    connect(dp, SIGNAL(downloadCompleted(QString)), this, SLOT(completeDownload(QString)));
    rotateWidget(la, dp, false);
    dp->download(url, this->isOnline);
}

void MainWindow::completeDownload(QString fileName)
{
    /* If we downloaded an image, replace former URL with an actual image file */
    if (fileName != NULL)
        this->image = QUrl(fileName);
#if defined (Q_OS_WIN) || defined (Q_OS_WIN32)
        /* Windows: we actually want the device ID */
        ep = new ExtractProgress(this, QString::number(this->installDeviceID), this->image.toString());
#endif
#if defined (Q_OS_MAC) || defined (Q_OS_LINUX)
        ep = new ExtractProgress(this, this->installDevicePath, this->image.toString());
#endif
    connect(ep, SIGNAL(finishedExtraction()), this, SLOT(showSuccessDialog()));
    rotateWidget(dp, ep, false);
    ep->extract();
}

void MainWindow::translate(QString locale)
{
    utils::writeLog("Attempting to load translation for locale " + locale);
    if (translator.load(qApp->applicationDirPath() + "/osmc_" + locale + ".qm"))
    {
        utils::writeLog("Translation loaded successfully");
        qApp->installTranslator(&translator);
        ui->retranslateUi(this);
        this->localeName = locale;
    }
    else
        utils::writeLog("Could not load translation!");
}

void MainWindow::showSuccessDialog()
{
    if (! this->device.allowsPreseedingNFS() && ! this->device.allowsPreseedingUSB() && ! this->device.allowsPreseedingSD() && ! this->device.allowsPreseedingInternal() && ! this->device.allowsPreseedingPartitioning() && ! this->device.allowsPreseedingNetwork())
    {
        sd = new SuccessDialog(this);
        rotateWidget(ep, sd, false);
    }
    else
    {
    /* Set up preseeder first */
    utils::writeLog("Creating preseeder");
    Preseeder *ps = new Preseeder();
      if (!this->localeName.isEmpty())
            ps->setLanguageString(this->localeName);
      ps->setTargetSettings(this);
      if (this->device.allowsPreseedingNetwork())
      {
         ps->setNetworkSettings(nss);
      }
    QStringList preseedList = ps->getPreseed();

    /* Write to the target */
    utils::writeLog("Writing preseeder");
#if defined (Q_OS_MAC) || defined (Q_OS_LINUX)
    QString diskPath;
    #if defined (Q_OS_LINUX)
        io::updateKernelTable();
        diskPath = nd->getDiskPath();
        if(diskPath.contains("mmcblk")) {
            diskPath += "p1";
        }
        else {
            diskPath += "1";
        }
    #endif
    #if defined (Q_OS_MAC)
        diskPath = nd->getDiskPath() + "s1";
    #endif

    /* Always first partition */
    utils::writeLog("Mounting the first filesystem on " + nd->getDiskPath());
    QDir mountDir = QDir(QDir::temp().absolutePath().append(QByteArray("/osmc_mnt")));
    if (! mountDir.exists())
        mountDir.mkpath(mountDir.absolutePath());

    utils::writeLog("Trying to umount before we are remounting and writing the preseed.");

    /* try both paths for umount, ignore errors */
    io::unmount(diskPath, false);
    io::unmount(nd->getDiskPath(), true);

    utils::writeLog("Mounting " + diskPath + " to " + mountDir.absolutePath());
    if (! io::mount(diskPath, mountDir.absolutePath()))
    {
        utils::writeLog("Could not mount filesystem!");
        utils::displayError(tr("Mount failed!"), tr("Could not mount device ") + diskPath + ". " + tr("Check the log for error messages. OSMC must exit now."), true);
        QApplication::quit();
        return;
    }
    else
    {
        utils::writeLog("Filesystem is mounted");
        utils::writeLog("Writing the preseeder to filesystem");
        QFile preseedFile(QString(mountDir.absolutePath() + "/preseed.cfg"));
        preseedFile.open(QIODevice::WriteOnly | QIODevice::Text);
    #ifdef Q_OS_LINUX
        // Set the owner and group the same as the home path
        QFileInfo info(QDir::homePath());
        fchown(preseedFile.handle(),info.ownerId(),info.groupId());
    #endif
        QTextStream out(&preseedFile);
        for (int i = 0; i < preseedList.count(); i++)
        {
            out << preseedList.at(i) + "\n";
        }
        out.flush();
        preseedFile.flush();
        preseedFile.close();
    }
    utils::writeLog("Finished. Syncing...");
    system("/bin/sync");
    utils::writeLog("Unmount in any case...");
    io::unmount(diskPath, false);
    utils::writeLog("Final sync.");
	system("/bin/sync");
#endif
#if defined (Q_OS_WIN) || defined (Q_OS_WIN32)
      /* We don't need to mount a partition here, thanks Windows.
       * But we might have not had a device path earlier due to no partition existing, so we need to re-run usbitcmd.exe and
       * check for matching device ID */
    int deviceID = nd->getDiskID();
    utils::writeLog("Trying to find device with id " + deviceID);
    QList<DiskDevice *> devices = io::enumerateDevice();
    for (int i = 0; i < devices.count(); i++)
    {
        if (devices.at(i)->getDiskID() == deviceID)
        {
            utils::writeLog("Matched device to entry point " + devices.at(i)->getDiskPath());
            utils::writeLog("Writing the preseeder to filesystem");
            QFile preseedFile(QString(devices.at(i)->getDiskPath() + "preseed.cfg"));
            preseedFile.open(QIODevice::WriteOnly | QIODevice::Text);
            QTextStream out(&preseedFile);
            for (int i = 0; i < preseedList.count(); i++)
            {
                out << preseedList.at(i) + "\n";
            }
            preseedFile.close();
            utils::writeLog("Finished.");
        }
    }
#endif
    sd = new SuccessDialog(this);
    rotateWidget(ep, sd, false);
}
}

MainWindow::~MainWindow()
{
    delete ui;
}
