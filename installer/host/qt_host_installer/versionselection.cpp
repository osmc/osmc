#include "versionselection.h"
#include "ui_versionselection.h"
#include "supporteddevice.h"
#include <QFileDialog>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include "utils.h"
#include <QMap>
#include <QDebug>

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
    while (reply->canReadLine())
    {
        enumerateBuilds(reply->readLine());
    }
    reply->deleteLater();
}

void VersionSelection::enumerateBuilds(QByteArray buildline)
{
    QString line = QString::fromUtf8(buildline);
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
}

VersionSelection::~VersionSelection()
{
    delete ui;
}

void VersionSelection::on_versionnextButton_clicked()
{
    if (ui->versionSelectionBox->isEnabled() && ui->versionSelectionBox->currentIndex() == 0)
    {
        utils::displayError(tr("Select a version"), tr("Please select a version!"));
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

void VersionSelection::on_useLocalBuildCheckbox_stateChanged(int arg1)
{
    if (ui->useLocalBuildCheckbox->isChecked())
    {
        QString dir = QDir::homePath();

        /* Dialog for selecting custom build */
        buildName = QString(); /* NULL it */
        buildName = QFileDialog::getOpenFileName(this, "Select disk image", dir, tr("OSMC Disk Images (**.img.gz)"));
        buildName.remove("file://");
        if (buildName == NULL)
        {
            utils::displayError(tr("Build selection error"), tr("You didn't select a custom build -- reverting to online builds"));
            ui->useLocalBuildCheckbox->setChecked(false);
        }
        else
        {
            ui->versionSelectionBox->setEnabled(false);
            ui->useLocalBuildCheckbox->setChecked(true);
        }
    }
    else
    {
        ui->versionSelectionBox->setEnabled(true);
    }
}
