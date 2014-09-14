#include "preseeder.h"
#include "utils.h"
#include <QStringList>
#include "networksettings.h"

Preseeder::Preseeder()
{
    preseedStringList = QStringList();
}

void Preseeder::setLanguageString(QString language)
{

}

void Preseeder::setNetworkSettings(NetworkSettings *ns)
{

}

void Preseeder::writeOption(QString preseedSection, QString preseedOptionKey, int preseedOptionType, QString preseedOptionValue)
{
    QString toWrite;
    toWrite = "d-i" + preseedSection + "/" + preseedOptionKey;
    switch (preseedOptionType)
    {
    case PRESEED_STRING:
        toWrite += "string";
        break;
    case PRESEED_BOOL:
        toWrite += "boolean";
        break;
    }
    toWrite += preseedOptionValue;
    utils::writeLog("Adding preseed string" + toWrite);
    preseedStringList << toWrite;
}

