#include "wifinetworksetup.h"
#include "ui_wifinetworksetup.h"
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
    /* TODO: Add validation */
    int key_type;
    key_type = (ui->keySelectionBox->currentText() == tr("Open Network")) ? utils::WIRELESS_ENCRYPTION_NONE : (ui->keySelectionBox->currentText() == tr("WPA/WPA2 PSK")) ? utils::WIRELESS_ENCRYPTION_WPAPSK : utils::WIRELESS_ENCRYPTION_WEP;
    emit wifiNetworkConfigured(ui->ssidnamelineEdit->text(), key_type, ui->keylineEdit->text());
}

void WiFiNetworkSetup::on_keySelectionBox_currentIndexChanged(const QString &arg1)
{
    if (arg1 != tr("Open Network"))
        ui->keylineEdit->setEnabled(true);
    else
        ui->keylineEdit->setEnabled(false);
}
