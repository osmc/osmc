#include "updatenotification.h"
#include "ui_updatenotification.h"
#include "utils.h"
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>

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
    #ifdef Q_OS_WIN32 || #ifdef Q_OS_WIN
    platform = QString("windows");
    #endif
    #ifdef Q_OS_LINUX
    platform = QString("linux");
    #endif
    QString appendURL = "<a href=\"http://osmc.tv/download/" + platform + "\"><span style=\" text-decoration: underline; color:#f0f0f0;\">http://osmc.tv/download/" + platform + "</span></a></p></body></html>";
    ui->downloadLinkLabel->setText(QString(ui->downloadLinkLabel->text() + appendURL));
    #ifdef Q_OS_LINUX
    /*TODO: Only display if /usr/bin/apt-get exists */
    ui->platformtipLabel->setText(QString("You can also do this with \"apt-get upgrade\""));
    #endif
}

UpdateNotification::~UpdateNotification()
{
    delete ui;
}

bool UpdateNotification::isUpdateAvailable()
{
    utils::writeLog("Checking for updates");
    int currentBuild = utils::getBuildNumber();
    QString buildURL = QString("http://download.osmc.tv/installers/latest_" + platform);
    QNetworkAccessManager accessManager;
    QNetworkRequest networkRequest(buildURL);
    QNetworkReply *networkReply = accessManager.get(networkRequest);
    if (!networkReply->error())
    {
        int latestBuild = networkReply->readAll().toInt();
        if (currentBuild < latestBuild)
        {
            utils::writeLog("A new update is available");
            return 1;
        }
        else
        {
            utils::writeLog("No new update is available");
            return 0;
        }
    }
    else
    {
        utils::writeLog("Error occurred trying to download " + buildURL);
        return 0;
    }
}
