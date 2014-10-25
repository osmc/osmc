#include "preseedparser.h"
#include <QFile>
#include <QTextStream>
#include <QStringList>
#include "utils.h"
#define PRESEED_FILE "/mnt/boot/preseed.cfg"

PreseedParser::PreseedParser()
{
    /* Attempt to load preseeder */
    QFile preseedFile(PRESEED_FILE);
    if (preseedFile.exists())
    {
        if (preseedFile.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            /* Load the preseeder contents */
            QTextStream preseedStream(&preseedFile);
            QString preseedString = preseedStream.readAll();
            preseedFile.close();
            this->preseedStringList = preseedString.split(QRegExp("[\r\n]"),QString::SkipEmptyParts);
            this->hasPreseed = true;
        }
        else
            this->hasPreseed = false;
    }
}

QString PreseedParser::getStringValue(QString desiredSetting)
{
    for (int i = 0; i < preseedStringList.count(); i++)
    {
        QString pString = preseedStringList.at(i);
        QStringList pStringSplit = pString.split(" ");
        if (pString.contains(desiredSetting))
            return pStringSplit.at(3);
    }
}

bool PreseedParser::getBoolValue(QString desiredSetting)
{
    for (int i = 0; i < preseedStringList.count(); i++)
    {
        QString pString = preseedStringList.at(i);
        QStringList pStringSplit = pString.split(" ");
        if (pString.contains(desiredSetting))
            return (pStringSplit.at(3) == "true") ? true : false;
    }
}
