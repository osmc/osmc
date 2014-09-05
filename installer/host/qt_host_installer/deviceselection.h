#ifndef DEVICESELECTION_H
#define DEVICESELECTION_H

#include <QWidget>

namespace Ui {
class DeviceSelection;
}

class DeviceSelection : public QWidget
{
    Q_OBJECT
    
public:
    explicit DeviceSelection(QWidget *parent = 0);
    ~DeviceSelection();
    
private:
    Ui::DeviceSelection *ui;
};

#endif // DEVICESELECTION_H
