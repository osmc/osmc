/*
 * (c) 2014-2015 Sam Nazarko
 * email@samnazarko.co.uk
*/
#ifndef TARGETLIST_H
#define TARGETLIST_H
#include <QMap>
#include <QString>
#include "target.h"

class TargetList
{
public:
    TargetList();
    ~TargetList();
    Target *getTarget(QString deviceKey) { return targetMap.value(deviceKey, NULL); }
private:
    QMap<QString, Target*> targetMap;
};

#endif // TARGETLIST_H
