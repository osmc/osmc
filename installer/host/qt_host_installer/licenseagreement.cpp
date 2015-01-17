/*
 * (c) 2014-2015 Sam Nazarko
 * email@samnazarko.co.uk
*/
#include "licenseagreement.h"
#include "utils.h"

LicenseAgreement::LicenseAgreement(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LicenseAgreement)
{
    ui->setupUi(this);
}

LicenseAgreement::~LicenseAgreement()
{
    delete ui;
}

void LicenseAgreement::on_licenseAcceptNextButton_clicked()
{
    ui->licenseAcceptNextButton->setEnabled(false);
    if (! ui->datapolacceptcheckBox->isChecked() || !ui->gplacceptcheckBox->isChecked())
    {
        utils::displayError(tr("License agreement"), tr("You need to accept the license agreement and data collection policy to proceed"));
        ui->licenseAcceptNextButton->setEnabled(true);
    }
    else
    {
        utils::writeLog("GPL licence and data collection policy has been accepted");
        emit licenseAccepted();
    }
}
