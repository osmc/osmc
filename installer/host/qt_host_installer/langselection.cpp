#include "langselection.h"
#include "ui_langselection.h"
#include "utils.h"

LangSelection::LangSelection(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LangSelection)
{
    ui->setupUi(this);
    QStringList supportedDevices;
    supportedDevices << "Raspberry Pi";
    ui->deviceSelectionBox->addItems(supportedDevices);
}

LangSelection::~LangSelection()
{
    delete ui;
}

void LangSelection::on_languagenextButton_clicked()
{
    if (ui->languageSelectionBox->currentIndex() != 0 && ui->deviceSelectionBox->currentIndex() != 0)
        emit languageSelected(ui->languageSelectionBox->currentText(), ui->deviceSelectionBox->currentText());
    else
        utils::displayError(tr("Error"), tr("You need to select an option!"));
}
