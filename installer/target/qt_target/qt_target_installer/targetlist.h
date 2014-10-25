#ifndef TARGETLIST_H
#define TARGETLIST_H
#include <QMap>
#include <QString>
#include "target.h"

class TargetList
{
public:
    TargetList();
    Target *getTarget(QString deviceKey) { return targetMap.value(deviceKey); }
private:
    QMap<QString, Target*> targetMap;
};

#endif // TARGETLIST_H
