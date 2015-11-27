/*
 * (c) 2014-2015 Sam Nazarko
 * email@samnazarko.co.uk
*/
#ifndef UTILS_H
#define UTILS_H

#include <QString>
#include <QFile>
#include <target.h>
#include <logger.h>

#define MNT_BOOT "/mnt/boot"
#define MNT_ROOT "/mnt/root"

class Utils
{
public:
    Utils(Logger *logger);
    QString getOSMCDev();
    void rebootSystem();
    bool mklabel(QString device, bool isGPT);
    bool setflag(QString device, QString flag, bool on);
    bool mkpart(QString device, QString fstype, QString start, QString end);
    bool fmtpart(QString partition, QString fstype);
    void writeToFile(QFile &file, QStringList strings, bool append);
    bool mountPartition(Target *device, QString path);
    bool unmountPartition(Target *device, QString path);
    int getPartSize(QString device, QString fstype);
    void updateDevTable();

private:
    Logger *logger;
};

#endif
