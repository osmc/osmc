#ifndef UTILS_H
#define UTILS_H

#include <QString>
#include <QFile>
#include <target.h>

#define MNT_BOOT "/mnt/boot"
#define MNT_ROOT "/mnt/root"

namespace utils
{
    QString getOSMCDev();
    void rebootSystem();
    bool mklabel(QString device, bool isGPT);
    bool mkpart(QString device, QString fstype, QString start, QString end);
    bool fmtpart(QString partition, QString fstype);
    void writeToFile(QFile &file, QStringList strings, bool append);
    bool mountPartition(Target *device, QString path);
    int getPartSize(QString device, QString fstype);
}

#endif
