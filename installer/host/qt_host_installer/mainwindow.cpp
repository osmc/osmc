#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "langselection.h"
#include "ui_langselection.h"
#include "updatenotification.h"
#include "ui_updatenotification.h"
#include "versionselection.h"
#include "ui_versionselection.h"
#include "ui_preseeddevice.h"
#include "preseeddevice.h"
#include "ui_networksetup.h"
#include "networksetup.h"
#include "ui_deviceselection.h"
#include "deviceselection.h"
#include "utils.h"
#include <QString>
#include <QTranslator>
#include "supporteddevice.h"
#include "advancednetworksetup.h"
#include "ui_advancednetworksetup.h"
#include "wifinetworksetup.h"
#include "ui_wifinetworksetup.h"
#include "networksettings.h"
#include <QList>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#define WIDGET_START QPoint(10,110)

UpdateNotification *updater;
LangSelection *ls;
VersionSelection *vs;
PreseedDevice *ps;
NetworkSetup *ns;
DeviceSelection *ds;
AdvancedNetworkSetup *ans;
WiFiNetworkSetup *wss;

QTranslator translator;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    this->setFixedSize(this->size());
    ui->setupUi(this);
    /* Attempt auto translation */
    QString autolocale = QLocale::system().name();
    utils::writeLog("Detected locale as " + autolocale);
    translate(autolocale);
    /* Enumerating devices */
    QList<SupportedDevice *> devices = utils::buildDeviceList();
    ls = new LangSelection(this, devices);
    connect(ls, SIGNAL(languageSelected(QString, SupportedDevice)), this, SLOT(setLanguage(QString, SupportedDevice)));
    ls->move(WIDGET_START);
    /* Resolve a mirror URL */
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

void MainWindow::replyFinished(QNetworkReply *reply)
{
    QVariant mirrorRedirectUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    this->mirrorURL = mirrorRedirectUrl.toString();
    utils::writeLog("Resolved mirror to " + this->mirrorURL);
    reply->deleteLater();
    /* Check if an update exists */
    updater->isUpdateAvailable(mirrorURL);
}

void MainWindow::dismissUpdate()
{
    updater->hide();
    ls->show();
}

void MainWindow::showUpdate()
{
    updater->show();
    connect(updater, SIGNAL(ignoreUpdate()), this, SLOT(dismissUpdate()));
    updater->move(WIDGET_START);
    ls->hide();
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
        vs->move(WIDGET_START);
        vs->show();
        ls->hide();
}

void MainWindow::setVersion(bool isOnline, QUrl image)
{
    if (isOnline)
    {
        QString localImage = image.toString();
        localImage.replace("http://download.osmc.tv/", this->mirrorURL);
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
    ps->move(WIDGET_START);
    ps->show();
    vs->hide();
}

void MainWindow::setPreseed(int installType)
{
    this->installType = installType;
    if (device.allowsPreseedingNetwork())
    {
        ns = new NetworkSetup(this, (this->installType == utils::INSTALL_NFS) ? false : true);
        connect(ns, SIGNAL(setNetworkOptionsInit(bool,bool)), this, SLOT(setNetworkInitial(bool,bool)));
        ns->move(WIDGET_START);
        ns->show();
        ps->hide();
    }
    else
    {
        /* Straight to device selection */
        ds = new DeviceSelection(this);
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
        ans->move(WIDGET_START);
        ans->show();
        ns->hide();
    }
    if (!advanced && useWireless)
    {
        nss->setDHCP(true);
        nss->setWireless(true);
        wss = new WiFiNetworkSetup(this);
        connect(wss, SIGNAL(wifiNetworkConfigured(QString,int,QString)), this, SLOT(setWiFiConfiguration(QString,int,QString)));
        wss->move(WIDGET_START);
        wss->show();
        ns->hide();
    }
    if (!advanced && !useWireless)
    {
        nss->setDHCP(true);
        nss->setWireless(false);
        ds = new DeviceSelection(this);
        /* ADD CONNECT */
        ds->move(WIDGET_START);
        ds->show();
        ns->hide();
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
        wss->move(WIDGET_START);
        wss->show();
        ans->hide();
    }
    else
    {
        ds = new DeviceSelection(this);
        /* ADD CONNECT */
        ds->move(WIDGET_START);
        ds->show();
        ans->hide();
    }
}

void MainWindow::setWiFiConfiguration(QString ssid, int key_type, QString key_value)
{
    utils::writeLog("Wireless network configured with SSID " + ssid + " key value " + key_value);
    nss->setWirelessSSID(ssid);
    nss->setWirelessKeyType(key_type);
    /* No point if open */
    if (! nss->getWirelessKeyType() == 0)
        nss->setWirelessKeyValue(key_value);
    ds = new DeviceSelection(this);
    /* ADD CONNECT */
    ds->move(WIDGET_START);
    ds->show();
    wss->hide();
}

void MainWindow::translate(QString locale)
{
    utils::writeLog("Attempting to load translation for locale " + locale);
    if (translator.load(qApp->applicationDirPath() + "/osmc_" + locale + ".qm"))
    {
        utils::writeLog("Translation loaded successfully");
        qApp->installTranslator(&translator);
        ui->retranslateUi(this);
    }
    else
        utils::writeLog("Could not load translation!");
}

MainWindow::~MainWindow()
{
    delete ui;
}
