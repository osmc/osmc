#include "deviceselection.h"
#include "ui_deviceselection.h"

DeviceSelection::DeviceSelection(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DeviceSelection)
{
    ui->setupUi(this);
}

DeviceSelection::~DeviceSelection()
{
    delete ui;
}
