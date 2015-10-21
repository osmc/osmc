/*
 * (c) 2014-2015 Sam Nazarko
 * email@samnazarko.co.uk
*/
#include "targetlist.h"
#include <target.h>
#include <QString>
#include <QMap>

TargetList::TargetList()
{
    /* Populate supported devices */
    /* Raspberry Pi */
    Target *RBP = new Target("/dev/mmcblk0p1", false, "fat32", true, "/dev/mmcblk0p2", false);
    Target *VERO = new Target("/dev/mmcblk0p1", false, "fat32", true, "/dev/mmcblk0p2", false);
    Target *ATV = new Target("/dev/sdb1", false, "hfsplus", true, "/dev/sdb2", true);
    /* Add to map */
    /* We can use the same Target for both Pis, identical entries */
    targetMap.insert("rbp1", RBP);
    targetMap.insert("rbp2", RBP);
    targetMap.insert("vero1", VERO);
    targetMap.insert("atv", ATV);
}

TargetList::~TargetList()
{
    QMapIterator<QString, Target*> i(targetMap);
    while (i.hasNext())
    {
        i.next();
        delete i.value();
    }
    targetMap.clear();
}
