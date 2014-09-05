#ifndef NETWORKSETUP_H
#define NETWORKSETUP_H

#include <QWidget>

namespace Ui {
class NetworkSetup;
}

class NetworkSetup : public QWidget
{
    Q_OBJECT
    
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
