#include "targetlist.h"
#include <target.h>
#include <QString>
#include <QMap>

TargetList::TargetList()
{
    /* Populate supported devices */
    /* Raspberry Pi */
    Target *RBP = new Target("/dev/mmcblk0p1", "vfat", true, "/dev/mmcblk0p2");
    /* Add to map */
    targetMap.insert("rbp", RBP);
}
