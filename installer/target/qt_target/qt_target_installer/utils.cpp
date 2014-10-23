#include "utils.h"
#include "cmdlineparser.h"
#include <QString>
#include <cstdlib>
#include <QProcess>
#include <QFile>
#include <QTextStream>

namespace utils
{
QString getOSMCDev()
{
    char osmcdev[10];
    #ifdef Q_WS_QWS
    get_cmdline_option("osmcdev=", osmcdev, sizeof(osmcdev));
    return (QString(osmcdev).simplified());
    #else
    /* Pretend to be Pi on testing system */
    return("rbp");
    #endif
}
void rebootSystem()
{
    /* Only reboot on real system */
    #ifdef Q_WS_QWS
    system("/bin/sh -c \"/bin/echo 1 > /proc/sys/kernel/sysrq\"");
    system("/bin/sh -c \"/bin/echo b > /proc/sysrq-trigger\"");
    #endif
}

void inline updateDevTable()
{
    system("/sbin/partprobe");
}

bool mklabel(QString device, bool isGPT)
{
    #ifdef Q_WS_QWS
    QProcess partedProcess;
    if (isGPT)
        partedProcess.start("parted -s" + device.toLocal8Bit() + "mklabel gpt");
    else
        partedProcess.start("parted -s" + device.toLocal8Bit() + "mklabel msdos");
    partedProcess.waitForFinished();
    updateDevTable();
    return partedProcess.exitCode();
    #else
    return true;
    #endif
}

bool mkpart(QString device, QString fstype, QString start, QString end)
{
    #ifdef Q_WS_QWS
    QProcess partedProcess;
    partedProcess.start("parted -s " + device.toLocal8Bit() + "mkpart primary " + fstype + " " + start + " " + end);
    partedProcess.waitForFinished();
    updateDevTable();
    return partedProcess.exitCode();
    #else
    return true;
    #endif
}
bool fmtpart(QString partition, QString fstype)
{
    #ifdef Q_WS_QWS
    QProcess mkfsProcess;
    if (fstype == "ext4")
        mkfsProcess.start("mkfs.ext4 " + partition);
    else if (fstype == "vfat")
        mkfsProcess.start("mkfs.vfat " + partition);
    else if (fstype == "vfat32")
        mkfsProcess.start("mkfs.vfat -F 32 " + partition);
    mkfsProcess.waitForFinished();
    return mkfs.exitCode();
    #else
    return true;
    #endif
}
void writeToFile(QFile file, QStringList *strings, bool append)
{
    if (append)
        file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
    else
        file.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream stream(&file);
    for (int i = 0; i < strings->count(); i++)
    {
        stream << strings->at(i);
    }
    file.close();
}
}
