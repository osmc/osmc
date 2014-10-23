#ifndef UTILS_H
#define UTILS_H

#include <QString>
#include <QFile>

namespace utils
{
    QString getOSMCDev();
    void rebootSystem();
    bool mklabel(QString device, bool isGPT);
    bool mkpart(QString device, QString fstype, QString start, QString end);
    bool fmtpart(QString partition, QString fstype);
    void writeToFile(QFile file, QStringList *strings, bool append);
}

#endif
