#include "langselection.h"
#include "ui_langselection.h"
#include "utils.h"
#include <QDir>
#include <QStringList>
#include <QDebug>

QStringList translationfileNames;

LangSelection::LangSelection(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LangSelection)
{
    ui->setupUi(this);
    /* Set up list of devices */
    QStringList supportedDevices;
    supportedDevices << "Raspberry Pi";
    ui->deviceSelectionBox->addItems(supportedDevices);
    /* Set up list of languages */
    ui->languageSelectionBox->addItem("English");
    QDir dir(QApplication::applicationDirPath());
    translationfileNames = dir.entryList(QStringList("osmc_*.qm"));
    for (int i = 0; i < translationfileNames.size(); ++i)
    {
        QString locale;
        locale = translationfileNames[i];
        locale.truncate(locale.lastIndexOf('.'));
        locale.remove(0, locale.indexOf('_') + 1);
        ui->languageSelectionBox->addItem(QLocale::languageToString(QLocale(locale).language()));
    }
}

LangSelection::~LangSelection()
{
    delete ui;
}

void LangSelection::on_languagenextButton_clicked()
{
    if (ui->languageSelectionBox->currentIndex() != 0 && ui->deviceSelectionBox->currentIndex() != 0)
    {
        for (int i = 0; i < translationfileNames.size(); ++i)
        {
            QString locale;
            locale = translationfileNames[i];
            locale.truncate(locale.lastIndexOf('.'));
            locale.remove(0, locale.indexOf('_') + 1);
            if (QLocale::languageToString(QLocale(locale).language()) == ui->languageSelectionBox->currentText())
                emit languageSelected(ui->languageSelectionBox->currentText(), ui->deviceSelectionBox->currentText());
            else
            {
                //qDebug() << QLocale::languageToString(QLocale(locale).language() << " is not equal to " << ui->languageSelectionBox->currentText();
            }
        }
    }
    else
        utils::displayError(tr("Error"), tr("You need to select an option!"));
}
