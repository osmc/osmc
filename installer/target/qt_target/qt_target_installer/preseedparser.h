#ifndef PRESEEDPARSER_H
#define PRESEEDPARSER_H
#include <QString>
#include <QStringList>

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
