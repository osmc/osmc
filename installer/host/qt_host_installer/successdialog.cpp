#include "successdialog.h"
#include "ui_successdialog.h"

SuccessDialog::SuccessDialog(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SuccessDialog)
{
    ui->setupUi(this);
}

SuccessDialog::~SuccessDialog()
{
    delete ui;
}

void SuccessDialog::on_closeInstallerButton_clicked()
{
    qApp->exit();
}
