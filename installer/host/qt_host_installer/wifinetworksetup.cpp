#include "wifinetworksetup.h"
#include "ui_wifinetworksetup.h"
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
    key_type = (ui->keySelectionBox->currentText() == tr("Open Network")) ? 0 : (ui->keySelectionBox->currentText() == tr("WPA/WPA2 PSK")) ? 1 : 2;
    emit wifiNetworkConfigured(ui->ssidnamelineEdit->text(), key_type, ui->keylineEdit->text());
}

void WiFiNetworkSetup::on_keySelectionBox_currentIndexChanged(const QString &arg1)
{
    if (arg1 != tr("Open Network"))
        ui->keylineEdit->setEnabled(true);
    else
        ui->keylineEdit->setEnabled(false);
}
