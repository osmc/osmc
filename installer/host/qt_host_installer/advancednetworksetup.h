/*
 * (c) 2014-2015 Sam Nazarko
 * email@samnazarko.co.uk
*/
#ifndef ADVANCEDNETWORKSETUP_H
#define ADVANCEDNETWORKSETUP_H

#include <QWidget>
#include "ui_advancednetworksetup.h"

namespace Ui {
class AdvancedNetworkSetup;
}

class AdvancedNetworkSetup : public QWidget
{
    Q_OBJECT

    virtual void showEvent(QShowEvent *event)
    {
        ui->networkoptionsnextButton->setEnabled(true);
    }
    
public:
    explicit AdvancedNetworkSetup(QWidget *parent = 0);
    ~AdvancedNetworkSetup();
    
private slots:
    void on_networkoptionsnextButton_clicked();

signals:
    void advancednetworkSelected(QString, QString, QString, QString, QString);
private:
    Ui::AdvancedNetworkSetup *ui;
};

#endif // ADVANCEDNETWORKSETUP_H
