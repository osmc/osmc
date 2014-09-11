#ifndef SUCCESSDIALOG_H
#define SUCCESSDIALOG_H

#include <QWidget>

namespace Ui {
class SuccessDialog;
}

class SuccessDialog : public QWidget
{
    Q_OBJECT
    
public:
    explicit SuccessDialog(QWidget *parent = 0);
    ~SuccessDialog();
    
private slots:
    void on_closeInstallerButton_clicked();

private:
    Ui::SuccessDialog *ui;
};

#endif // SUCCESSDIALOG_H
