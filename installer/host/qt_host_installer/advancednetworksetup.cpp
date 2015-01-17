/*
 * (c) 2014-2015 Sam Nazarko
 * email@samnazarko.co.uk
*/
#include "advancednetworksetup.h"
#include "utils.h"

AdvancedNetworkSetup::AdvancedNetworkSetup(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AdvancedNetworkSetup)
{
    ui->setupUi(this);
    ui->ipaddresslineEdit->setInputMask("000.000.000.000;_");
    ui->masklineEdit->setInputMask("000.000.000.000;_");
    ui->gwlineEdit->setInputMask("000.000.000.000;_");
    ui->dns1lineEdit->setInputMask("000.000.000.000;_");
    ui->dns2lineEdit->setInputMask("000.000.000.000;_");
    ui->networkoptionsnextButton->setEnabled(true);
}

AdvancedNetworkSetup::~AdvancedNetworkSetup()
{
    delete ui;
}

void AdvancedNetworkSetup::on_networkoptionsnextButton_clicked()
{
    ui->networkoptionsnextButton->setEnabled(false);
    QString ipAddress = ui->ipaddresslineEdit->text();
    QString mask = ui->masklineEdit->text();
    QString gateway = ui->gwlineEdit->text();
    QString dns1 = ui->dns1lineEdit->text();
    QString dns2 = ui->dns2lineEdit->text();

    if (!utils::validateIp(ipAddress))
    {
        utils::displayError(tr("Invalid IP"), tr("You need to provide a valid IP address!"), false);
        ui->networkoptionsnextButton->setEnabled(true);
        return;
    }

    if (!utils::validateIp(mask))
    {
        utils::displayError(tr("Invalid Networkmask"), tr("You need to provide a valid network mask!"), false);
        ui->networkoptionsnextButton->setEnabled(true);
        return;
    }

    if (!utils::validateIp(gateway))
    {
        utils::displayError(tr("Invalid Gateway-address"), tr("You need to provide a valid address for the gateway!"), false);
        ui->networkoptionsnextButton->setEnabled(true);
        return;
    }

    if (!utils::validateIp(dns1))
    {
        utils::displayError(tr("Invalid DNS IP"), tr("You need to provide a valid IP address for DNS-1!"), false);
        ui->networkoptionsnextButton->setEnabled(true);
        return;
    }

    if (!utils::validateIp(dns2))
    {
        utils::displayError(tr("Invalid DNS IP"), tr("You need to provide a valid IP address for DNS-2!"), false);
        ui->networkoptionsnextButton->setEnabled(true);
        return;
    }


    emit advancednetworkSelected(ipAddress, mask, gateway, dns1, dns2);
}
