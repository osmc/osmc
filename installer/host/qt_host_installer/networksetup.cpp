/*
 * (c) 2014-2015 Sam Nazarko
 * email@samnazarko.co.uk
*/
#include "networksetup.h"

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
    ui->networkoptionsnextButton->setEnabled(false);
    if (ui->wirelessconnectionradioButton->isChecked())
        emit setNetworkOptionsInit(true, ui->advancednetworkingcheckBox->checkState());
    if (ui->wiredconnectionradioButton->isChecked())
        emit setNetworkOptionsInit(false, ui->advancednetworkingcheckBox->checkState());
}
