/*
 * (c) 2014-2015 Sam Nazarko
 * email@samnazarko.co.uk
*/
#include "versionselection.h"
#include "supporteddevice.h"
#include <QFileDialog>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include "utils.h"
#include <QMap>

VersionSelection::VersionSelection(QWidget *parent, QString deviceShortName, QString mirrorURL) :
    QWidget(parent),
    ui(new Ui::VersionSelection)
{
    ui->setupUi(this);
    /* List of available builds from the Internet */
    QUrl downloadsURL = QUrl(mirrorURL + "installers/versions_" + deviceShortName);
    utils::writeLog("Attempting to download device versions file " + downloadsURL.toString());
    accessManager = new QNetworkAccessManager(this);
    connect(accessManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));
    QNetworkRequest request(downloadsURL);
    accessManager->get(request);
}

void VersionSelection::replyFinished(QNetworkReply *reply)
{
    bool hasOSMCURL = false;
    if (reply->error() == QNetworkReply::NoError)
    {
        while (reply->canReadLine())
            if (enumerateBuilds(reply->readLine()))
                hasOSMCURL = true; /* Check if we get at least one build back. We may get 200 but not the expected page.. */
        reply->deleteLater();
    }
    else if (reply->error() != QNetworkReply::NoError || ! hasOSMCURL)
        utils::displayError(tr("Error connecting to network"), tr("There seems to be an issue connecting to the network") + "\n" + "\n" + tr("Please check that you are not trying to download OSMC via a proxy server"), false);
}

bool VersionSelection::enumerateBuilds(QByteArray buildline)
{
    QString line = QString::fromUtf8(buildline);
    if (line.contains("download.osmc.tv"))
    {
        QStringList splitline;
        QString buildnameline;
        splitline = line.split(QRegExp("\\ "));
        for (int i = 0; i < (splitline.count() - 1); i++)
        {
           buildnameline = buildnameline + " " + splitline.at(i);
        }
        utils::writeLog("Found a build called " + buildnameline);
        ui->versionSelectionBox->addItem(buildnameline);
        buildMap.insert(buildnameline, splitline.at((splitline.count() -1)));
        return true;
    }
}

VersionSelection::~VersionSelection()
{
    delete ui;
}

void VersionSelection::on_versionnextButton_clicked()
{
    ui->versionnextButton->setEnabled(false);
    if (ui->versionSelectionBox->isEnabled() && ui->versionSelectionBox->currentIndex() == 0)
    {
        utils::displayError(tr("Select a version"), tr("Please select a version!"));
        ui->versionnextButton->setEnabled(true);
    }
    else
    {
        if (ui->useLocalBuildCheckbox->isChecked())
        {
            emit versionSelected(false, QUrl::fromLocalFile(buildName));
        }
        else
        {
            emit versionSelected(true, buildMap.value(ui->versionSelectionBox->currentText()));
        }
    }
}

void VersionSelection::on_useLocalBuildCheckbox_stateChanged(int checkState)
{
    if (checkState == Qt::Checked)
    {
        QString dir = QDir::homePath();

        /* Dialog for selecting custom build */
        buildName = QString(); /* NULL it */
        buildName = QFileDialog::getOpenFileName(this, "Select disk image", dir, tr("OSMC Disk Images (**.img.gz)"));
        if (buildName == NULL)
        {
            utils::displayError(tr("Build selection error"), tr("You didn't select a custom build -- reverting to online builds"));
            ui->useLocalBuildCheckbox->setCheckState(Qt::Unchecked);
        }
        else
        {
            ui->versionSelectionBox->setEnabled(false);
            ui->useLocalBuildCheckbox->setCheckState(Qt::Checked);
        }
    }
    else
    {
        ui->versionSelectionBox->setEnabled(true);
    }
}
