/*
 * (c) 2014-2015 Sam Nazarko
 * email@samnazarko.co.uk
*/
#include "langselection.h"
#include "utils.h"
#include <QDir>
#include <QList>
#include "supporteddevice.h"
#include <QPalette>
#include <QColor>

QStringList translationfileNames;
QList<SupportedDevice *> devicesList;

LangSelection::LangSelection(QWidget *parent, QList<SupportedDevice *> devices) :
    QWidget(parent),
    ui(new Ui::LangSelection)
{
    ui->setupUi(this);
    devicesList = devices;
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
    /* Set up list of devices */
    utils::writeLog("Added the following devices");
    for (int i = 0; i < devicesList.count(); i++)
    {
        SupportedDevice *dev = devicesList.at(i);
        ui->deviceSelectionBox->addItem(dev->getDeviceName());
        utils::writeLog(dev->getDeviceName());
    }
    /* Look less ugly */
    QPalette boxPalette = ui->languageSelectionBox->palette();
    boxPalette.setColor(QPalette::HighlightedText, QColor(240, 240, 240));
    boxPalette.setColor(QPalette::Highlight, QColor(23, 57, 74));
    ui->languageSelectionBox->setPalette(boxPalette);
    ui->deviceSelectionBox->setPalette(boxPalette);
    ui->languagenextButton->setEnabled(true);
}

LangSelection::~LangSelection()
{
    delete ui;
}

void LangSelection::on_languagenextButton_clicked()
{
    ui->languagenextButton->setEnabled(false);
    if (ui->languageSelectionBox->currentIndex() != 0 && ui->deviceSelectionBox->currentIndex() != 0)
    {
        SupportedDevice *device;
        for (int i = 0; i < devicesList.count(); i++)
        {
            SupportedDevice *dev = devicesList.at(i);
            if (dev->getDeviceName() == ui->deviceSelectionBox->currentText())
                device = dev;
        }
        if (ui->languageSelectionBox->currentText() == tr("English"))
            emit languageSelected(tr("English"), *device);
        else
        {
            for (int i = 0; i < translationfileNames.size(); ++i)
            {
                QString locale;
                locale = translationfileNames[i];
                locale.truncate(locale.lastIndexOf('.'));
                locale.remove(0, locale.indexOf('_') + 1);
                if (QLocale::languageToString(QLocale(locale).language()) == ui->languageSelectionBox->currentText())
                    emit languageSelected(locale, *device);
            }
        }
    }
    else
    {
        utils::displayError(tr("Error"), tr("You need to select an option!"));
        ui->languagenextButton->setEnabled(true);
    }
}
