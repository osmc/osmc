#ifndef ADVANCEDNETWORKSETUP_H
#define ADVANCEDNETWORKSETUP_H

#include <QWidget>

namespace Ui {
class AdvancedNetworkSetup;
}

class AdvancedNetworkSetup : public QWidget
{
    Q_OBJECT
    
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
