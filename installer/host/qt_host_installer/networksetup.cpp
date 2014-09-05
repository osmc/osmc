#include "networksetup.h"
#include "ui_networksetup.h"

NetworkSetup::NetworkSetup(QWidget *parent, bool allowWireless) :
    QWidget(parent),
    ui(new Ui::NetworkSetup)
{
    ui->setupUi(this);
    if (!allowWireless)
        ui->wirelessconnectionradioButton->setEnabled(false);
}

NetworkSetup::~NetworkSetup()
{
    delete ui;
}

void NetworkSetup::on_networkoptionsnextButton_clicked()
{
    if (ui->wirelessconnectionradioButton->isChecked())
        emit setNetworkOptionsInit(true, ui->advancednetworkingcheckBox->checkState());
    if (ui->wiredconnectionradioButton->isChecked())
        emit setNetworkOptionsInit(false, ui->advancednetworkingcheckBox->checkState());
}
