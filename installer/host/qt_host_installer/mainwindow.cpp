#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "langselection.h"
#include "ui_langselection.h"
#include "updatenotification.h"
#include "ui_updatenotification.h"
#include "versionselection.h"
#include "ui_versionselection.h"
#include "utils.h"
#include <QString>
#include <QTranslator>
#include "supporteddevice.h"
#include <QList>
#include <QDebug>
#define WIDGET_START QPoint(10,110)

UpdateNotification *updater;
LangSelection *ls;
VersionSelection *vs;

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
    QList<SupportedDevice> *devices = utils::buildDeviceList();
    ls = new LangSelection(this, devices);
    connect(ls, SIGNAL(languageSelected(QString, SupportedDevice*)), this, SLOT(setLanguage(QString, SupportedDevice*)));
    ls->move(WIDGET_START);
    /* Check if an update exists */
    updater = new UpdateNotification(this);
    updater->hide();
    connect(updater, SIGNAL(hasUpdate()), this, SLOT(showUpdate()));
    updater->isUpdateAvailable();
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

void MainWindow::setLanguage(QString language, SupportedDevice *device)
{
        this->language = language;
        this->device = device;
        utils::writeLog("The user has selected " + this->language + " as their language");
        utils::writeLog("The user has selected " + this->device->getDeviceName() + " as their device");
        if (language != tr("English"))
        {
            translate(language);
        }
        else
        {
            /* Remove because we may have already done the deed */
            qApp->removeTranslator(&translator);
        }
        vs = new VersionSelection(this, device->getDeviceShortName());
        connect(vs, SIGNAL(versionSelected(bool, QUrl)), this, SLOT(setVersion(bool, QUrl)));
        vs->move(WIDGET_START);
        vs->show();
        ls->hide();
}

void MainWindow::setVersion(bool isOnline, QUrl image)
{
    if (isOnline)
    {
        utils::writeLog("The user has selected an online image for " + device->getDeviceName() + " build URL : " + image.toString());
        this->isOnline = true;
    }
    else
    {
        utils::writeLog("The user has selected a local image for " + device->getDeviceName() + "file location: " + image.toString());
        this->isOnline = false;
    }
    this->image = image;
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
