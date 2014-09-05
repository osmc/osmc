#ifndef INSTALLPROGRESS_H
#define INSTALLPROGRESS_H

#include <QWidget>

namespace Ui {
class InstallProgress;
}

class InstallProgress : public QWidget
{
    Q_OBJECT
    
public:
    explicit InstallProgress(QWidget *parent = 0);
    ~InstallProgress();
    
private:
    Ui::InstallProgress *ui;
};

#endif // INSTALLPROGRESS_H
