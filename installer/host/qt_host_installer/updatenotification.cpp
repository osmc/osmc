/*
 * (c) 2014-2015 Sam Nazarko
 * email@samnazarko.co.uk
*/
#include "updatenotification.h"
#include "ui_updatenotification.h"
#include "utils.h"
#include <QtNetwork/QNetworkReply>
#include <QFile>

QString platform;

UpdateNotification::UpdateNotification(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::UpdateNotification)
{
    ui->setupUi(this);
    ui->downloadLinkLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    ui->downloadLinkLabel->setOpenExternalLinks(true);
    /* Add link here, not in designer, so our translators don't need to touch HTML */
    #ifdef Q_OS_MAC
    platform = QString("mac");
    #endif
    #if defined(Q_OS_WIN32) || defined(Q_OS_WIN)
    platform = QString("windows");
    #endif
    #ifdef Q_OS_LINUX
    platform = QString("linux");
    #endif
    QString appendURL = "<a href=\"http://osmc.tv/download/" + platform + "\"><span style=\" text-decoration: underline; color:#51BC9B;\">http://osmc.tv/download/" + platform + "</span></a></p></body></html>";
    ui->downloadLinkLabel->setText(QString(ui->downloadLinkLabel->text() + appendURL));
    #ifdef Q_OS_LINUX
    /*TODO: Only display if /usr/bin/apt-get exists */
    QFile aptFile("/usr/bin/apt-get");
    QFile yumFile("/usr/bin.yum");
    if (aptFile.exists())
        ui->platformtipLabel->setText(QString("You can also do this with \"apt-get update && apt-get upgrade\""));
    if (yumFile.exists())
        ui->platformtipLabel->setText(QString("You can also do this with \"yum update\""));
    #endif
}

UpdateNotification::~UpdateNotification()
{
    delete ui;
}

void UpdateNotification::isUpdateAvailable(QString &baseURL)
{   
    utils::writeLog("Checking for updates");
    int currentBuild = utils::getBuildNumber();
    QString buildURL = QString(baseURL + "/installers/latest_" + platform);
    utils::writeLog("Checking for updates by downloading " + buildURL);
    accessManager = new QNetworkAccessManager(this);
    connect(accessManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));
    QNetworkRequest request(buildURL);
    accessManager->get(request);
}

void UpdateNotification::replyFinished(QNetworkReply *reply)
{
   utils::writeLog("Acquired mirror file");
   int latestBuild = QString::fromUtf8(reply->readAll()).toInt();
   if (utils::getBuildNumber() < latestBuild)
   {
       utils::writeLog("A new update is available");
       emit hasUpdate();
   }
   else
   {
       utils::writeLog("No new update is available");
   }
   reply->deleteLater();
}
