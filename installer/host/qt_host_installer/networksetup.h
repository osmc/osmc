/*
 * (c) 2014-2015 Sam Nazarko
 * email@samnazarko.co.uk
*/
#ifndef NETWORKSETUP_H
#define NETWORKSETUP_H

#include <QWidget>
#include "ui_networksetup.h"

namespace Ui {
class NetworkSetup;
}

class NetworkSetup : public QWidget
{
    Q_OBJECT
    
    virtual void showEvent(QShowEvent *event)
    {
        ui->networkoptionsnextButton->setEnabled(true);
    }

public:
    explicit NetworkSetup(QWidget *parent = 0, bool allowWireless = 1);
    ~NetworkSetup();
    
private slots:

    void on_networkoptionsnextButton_clicked();

signals:
    void setNetworkOptionsInit(bool, bool);

private:
    Ui::NetworkSetup *ui;
};

#endif // NETWORKSETUP_H
