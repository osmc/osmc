#include "langselection.h"
#include "ui_langselection.h"
#include "utils.h"
#include <QDir>
#include <QList>
#include "installabledevices.h"
#include "device.h"
#include <QPalette>
#include <QColor>

QStringList translationfileNames;

LangSelection::LangSelection(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LangSelection)
{
    ui->setupUi(this);
    /* Set up list of devices */
    QList<Device> supportedDevices = InstallableDevices::getDevices();
    /* Set up list of languages */
    ui->languageSelectionBox->addItem(tr("English"));
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
    /* Look less ugly */
    QPalette boxPalette = ui->languageSelectionBox->palette();
    boxPalette.setColor(QPalette::HighlightedText, QColor(240, 240, 240));
    boxPalette.setColor(QPalette::Highlight, QColor(23, 57, 74));
    ui->languageSelectionBox->setPalette(boxPalette);
    ui->deviceSelectionBox->setPalette(boxPalette);
}

LangSelection::~LangSelection()
{
    delete ui;
}

void LangSelection::on_languagenextButton_clicked()
{
    if (ui->languageSelectionBox->currentIndex() != 0 && ui->deviceSelectionBox->currentIndex() != 0)
    {
        if (ui->languageSelectionBox->currentText() == tr("English"))
            emit languageSelected(tr("English"), ui->deviceSelectionBox->currentText());
        else
        {
            for (int i = 0; i < translationfileNames.size(); ++i)
            {
                QString locale;
                locale = translationfileNames[i];
                locale.truncate(locale.lastIndexOf('.'));
                locale.remove(0, locale.indexOf('_') + 1);
                if (QLocale::languageToString(QLocale(locale).language()) == ui->languageSelectionBox->currentText())
                    emit languageSelected(locale, ui->deviceSelectionBox->currentText());
            }
        }
    }
    else
        utils::displayError(tr("Error"), tr("You need to select an option!"));
}

