#ifndef LICENSEAGREEMENT_H
#define LICENSEAGREEMENT_H

#include <QWidget>

namespace Ui {
class LicenseAgreement;
}

class LicenseAgreement : public QWidget
{
    Q_OBJECT
    
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
