/*
 * (c) 2014-2015 Sam Nazarko
 * email@samnazarko.co.uk
*/
#include "wifinetworksetup.h"
#include "utils.h"

WiFiNetworkSetup::WiFiNetworkSetup(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WiFiNetworkSetup)
{
    ui->setupUi(this);
    ui->keySelectionBox->addItem(tr("Open Network"));
    ui->keySelectionBox->addItem(tr("WPA/WPA2 PSK"));
    ui->keySelectionBox->addItem(tr("WEP"));
    ui->keylineEdit->setEnabled(false);
}

WiFiNetworkSetup::~WiFiNetworkSetup()
{
    delete ui;
}

void WiFiNetworkSetup::on_networkoptionsnextButton_clicked()
{
    ui->networkoptionsnextButton->setEnabled(false);
    if (ui->keySelectionBox->currentText().startsWith("---"))
    {
        utils::displayError(tr("Networktype missing"), tr("You need to select the type of network encryption!"), false);
        ui->networkoptionsnextButton->setEnabled(true);
        return;
    }

    int key_type;
    key_type = (ui->keySelectionBox->currentText() == tr("Open Network")) ? utils::WIRELESS_ENCRYPTION_NONE : (ui->keySelectionBox->currentText() == tr("WPA/WPA2 PSK")) ? utils::WIRELESS_ENCRYPTION_WPAPSK : utils::WIRELESS_ENCRYPTION_WEP;

    QString ssid = ui->ssidnamelineEdit->text();
    QString key = ui->keylineEdit->text();

    if (ssid.isEmpty())
    {
        utils::displayError(tr("Missing SSID"), tr("You need to provide a SSID!"), false);
        ui->networkoptionsnextButton->setEnabled(true);
        return;
    }

    switch (key_type)
    {
    case (utils::WIRELESS_ENCRYPTION_WEP):
    case (utils::WIRELESS_ENCRYPTION_WPAPSK):
        if (key.isEmpty())
        {
            utils::displayError(tr("Missing Key"), tr("You need to provide a key!"), false);
            ui->networkoptionsnextButton->setEnabled(true);
            return;
        }
    }

    emit wifiNetworkConfigured(ssid, key_type, key);
}

void WiFiNetworkSetup::on_keySelectionBox_currentIndexChanged(const QString &arg1)
{
    if (arg1 != tr("Open Network"))
        ui->keylineEdit->setEnabled(true);
    else
        ui->keylineEdit->setEnabled(false);
}
