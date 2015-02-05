/*
 * (c) 2014-2015 Sam Nazarko
 * email@samnazarko.co.uk
*/
#ifndef LICENSEAGREEMENT_H
#define LICENSEAGREEMENT_H

#include <QWidget>
#include "ui_licenseagreement.h"

namespace Ui {
class LicenseAgreement;
}

class LicenseAgreement : public QWidget
{
    Q_OBJECT
    
    virtual void showEvent(QShowEvent *event)
    {
        ui->licenseAcceptNextButton->setEnabled(true);
    }

public:
    explicit LicenseAgreement(QWidget *parent = 0);
    ~LicenseAgreement();
    
private slots:

    void on_licenseAcceptNextButton_clicked();

signals:
    void licenseAccepted();

private:
    Ui::LicenseAgreement *ui;
};

#endif // LICENSEAGREEMENT_H
