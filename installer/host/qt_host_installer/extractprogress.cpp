#include "extractprogress.h"
#include "ui_extractprogress.h"

ExtractProgress::ExtractProgress(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ExtractProgress)
{
    ui->setupUi(this);
}

ExtractProgress::~ExtractProgress()
{
    delete ui;
}
