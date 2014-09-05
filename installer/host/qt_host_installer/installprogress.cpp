#include "installprogress.h"
#include "ui_installprogress.h"

InstallProgress::InstallProgress(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::InstallProgress)
{
    ui->setupUi(this);
}

InstallProgress::~InstallProgress()
{
    delete ui;
}
