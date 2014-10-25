#include "utils.h"
#include "cmdlineparser.h"
#include <QString>
#include <cstdlib>
#include <QProcess>
#include <QFile>
#include <QTextStream>
#include <target.h>
#include <sys/mount.h>

namespace utils
{
QString getOSMCDev()
{
    char osmcdev[10];
    get_cmdline_option("osmcdev=", osmcdev, sizeof(osmcdev));
    return (QString(osmcdev).simplified());
}
void rebootSystem()
{
    /* Only reboot on real system */
    system("/bin/sh -c \"/bin/echo 1 > /proc/sys/kernel/sysrq\"");
    system("/bin/sh -c \"/bin/echo b > /proc/sysrq-trigger\"");
}

void inline updateDevTable()
{
    system("/sbin/partprobe");
}

bool mklabel(QString device, bool isGPT)
{
    QProcess partedProcess;
    if (isGPT)
        partedProcess.start("parted -s" + device.toLocal8Bit() + "mklabel gpt");
    else
        partedProcess.start("parted -s" + device.toLocal8Bit() + "mklabel msdos");
    partedProcess.waitForFinished();
    updateDevTable();
    return partedProcess.exitCode();
}

bool mkpart(QString device, QString fstype, QString start, QString end)
{
    QProcess partedProcess;
    partedProcess.start("parted -s " + device.toLocal8Bit() + "mkpart primary " + fstype + " " + start + " " + end);
    partedProcess.waitForFinished();
    updateDevTable();
    return partedProcess.exitCode();
}
bool fmtpart(QString partition, QString fstype)
{
    QProcess mkfsProcess;
    if (fstype == "ext4")
        mkfsProcess.start("mkfs.ext4 " + partition);
    else if (fstype == "vfat")
        mkfsProcess.start("mkfs.vfat " + partition);
    else if (fstype == "vfat32")
        mkfsProcess.start("mkfs.vfat -F 32 " + partition);
    mkfsProcess.waitForFinished();
    return mkfsProcess.exitCode();
}
void writeToFile(QFile &file, QStringList strings, bool append)
{    
    if (append)
        file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
    else
        file.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream stream(&file);
    for (int i = 0; i < strings.count(); i++)
    {
        stream << strings.at(i);
    }
    file.close();
}
bool mountPartition(Target *device, QString path)
{
    system("mkdir -p " + path.toLocal8Bit());
    if (path == MNT_BOOT)
    {
        return (mount(device->getBoot().toLocal8Bit(), MNT_BOOT, device->getBootFS().toLocal8Bit(), (device->isBootRW() == true) ? 0 : 1, "") == 0) ? true : false;
    }
    else if (path == MNT_ROOT)
    {
        if (device->getRoot().contains(":/") && device->hasRootChanged())
        {
            /* This is an NFS share, use BusyBox */
            QProcess mountProcess;
            mountProcess.start("mount -t nfs -o nolock,noatime " + device->getRoot().toLocal8Bit() + " " +  MNT_ROOT);
            if (mountProcess.exitCode() == 0)
                return true;
            else
                return false;
        }
        else
        {
            return (mount(device->getRoot().toLocal8Bit(), MNT_ROOT, "ext4", 0, "") == 0) ? true : false;
        }
    }
}
}
