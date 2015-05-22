/*
 * (c) 2014-2015 Sam Nazarko
 * email@samnazarko.co.uk
*/
#include "successdialog.h"
#include "ui_successdialog.h"
#include <QDesktopServices>
#include <QUrl>

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

void SuccessDialog::on_facebookButton_clicked()
{
    QDesktopServices::openUrl(QUrl("http://facebook.com/osmcproject"));
}

void SuccessDialog::on_twitterButton_clicked()
{
    QDesktopServices::openUrl(QUrl("http://twitter.com/try_osmc"));
}
