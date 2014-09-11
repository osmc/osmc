#ifndef EXTRACTPROGRESS_H
#define EXTRACTPROGRESS_H

#include <QWidget>

namespace Ui {
class ExtractProgress;
}

class ExtractProgress : public QWidget
{
    Q_OBJECT
    
public:
    explicit ExtractProgress(QWidget *parent = 0);
    ~ExtractProgress();
    
private:
    Ui::ExtractProgress *ui;
};

#endif // EXTRACTPROGRESS_H
