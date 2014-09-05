#include "advancednetworksetup.h"
#include "ui_advancednetworksetup.h"

AdvancedNetworkSetup::AdvancedNetworkSetup(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AdvancedNetworkSetup)
{
    ui->setupUi(this);
}

AdvancedNetworkSetup::~AdvancedNetworkSetup()
{
    delete ui;
}

void AdvancedNetworkSetup::on_networkoptionsnextButton_clicked()
{
    /* TODO: Add validation */
    emit advancednetworkSelected(this->ui->ipaddresslineEdit->text(), ui->masklineEdit->text(), ui->gatewayLabel->text(), ui->dns1lineEdit->text(), ui->dns2lineEdit->text());
}
