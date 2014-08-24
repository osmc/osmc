#include "versionselection.h"
#include "ui_versionselection.h"
#include "supporteddevice.h"
#include <QFileDialog>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include "utils.h"

VersionSelection::VersionSelection(QWidget *parent, QString deviceShortName) :
    QWidget(parent),
    ui(new Ui::VersionSelection)
{
    ui->setupUi(this);
    /* List of available builds from the Internet */
    QString downloadsURL = QString("http://download.osmc.tv/installers/versions_" + deviceShortName);
    utils::writeLog("Attempting to download device versions file " + downloadsURL);
    QNetworkAccessManager accessManager;
    QNetworkRequest networkRequest(downloadsURL);
    QNetworkReply *networkReply = accessManager.get(networkRequest);
    while (networkReply->canReadLine())
    {
        ui->versionSelectionBox->addItem(networkReply->readLine());
    }
    if (ui->versionSelectionBox->count() == 1)
    {
        utils::writeLog("Could not enumerate the version selection dialog box. Likely a network error");
    }
}

VersionSelection::~VersionSelection()
{
    delete ui;
}

void VersionSelection::on_versionnextButton_clicked()
{
    if (ui->versionSelectionBox->isEnabled() == false && ui->versionSelectionBox->currentIndex() == 0)
    {
        utils::displayError(tr("Select a version"), tr("Please select a version!"));
    }
}

void VersionSelection::on_useLocalBuildCheckbox_stateChanged(int arg1)
{
    if (ui->useLocalBuildCheckbox->isChecked())
    {
        /* Dialog for selecting custom build */
        QString buildName = QFileDialog::getOpenFileName(this, "Select disk image", "", tr("OSMC Disk Images (*.*)"));
        if (buildName == NULL)
        {
            utils::displayError(tr("Build selection error"), tr("You didn't select a custom build -- reverting to online builds"));
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
