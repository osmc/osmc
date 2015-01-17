/*
 * (c) 2014-2015 Sam Nazarko
 * email@samnazarko.co.uk
*/
#ifndef PRESEEDPARSER_H
#define PRESEEDPARSER_H
#include <QString>
#include <QStringList>
#define PRESEED_FILE "/mnt/boot/preseed.cfg"

class PreseedParser
{
public:
    PreseedParser();
    bool isLoaded() { return hasPreseed; }
    QString getStringValue(QString desiredSetting);
    bool getBoolValue(QString desiredSetting);

private:
    bool hasPreseed = false;
    QStringList preseedStringList;
};

#endif // PRESEEDPARSER_H
