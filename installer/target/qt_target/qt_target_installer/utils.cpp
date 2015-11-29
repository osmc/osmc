/*
 * (c) 2014-2015 Sam Nazarko
 * email@samnazarko.co.uk
*/
#include "utils.h"
#include "cmdlineparser.h"
#include <QString>
#include <cstdlib>
#include <QProcess>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <target.h>
#include <sys/mount.h>

Utils::Utils(Logger *logger)
{
    this->logger = logger;
}

QString Utils::getOSMCDev()
{
    char osmcdev[10];
    get_cmdline_option("osmcdev=", osmcdev, sizeof(osmcdev));
    return (QString(osmcdev).simplified());
}
void Utils::rebootSystem()
{
    /* Only reboot on real system */
    system("/bin/sh -c \"/bin/echo 1 > /proc/sys/kernel/sysrq\"");
    system("/bin/sync");
    system("/bin/sh -c \"/bin/sleep 10 && /bin/echo b > /proc/sysrq-trigger\"");
}

void Utils::updateDevTable()
{
    system("/usr/sbin/partprobe");
}

bool Utils::mklabel(QString device, bool isGPT)
{
    QProcess mklabelProcess;
    logger->addLine("Going to mklabel with device: " + device + " with a label type of " + ((isGPT == true) ? "GPT" : "MSDOS"));
    if (isGPT)
        mklabelProcess.start("/usr/sbin/parted -s " + device.toLocal8Bit() + " mklabel gpt");
    else
        mklabelProcess.start("/usr/sbin/parted -s " + device.toLocal8Bit() + " mklabel msdos");
    mklabelProcess.waitForFinished(-1);
    updateDevTable();
    return mklabelProcess.exitCode() == 0;
}

bool Utils::setflag(QString device, QString flag, bool on)
{
    QProcess setflagProcess;
    logger->addLine("Going to set flag " + flag + " on " + device + " with value of " + (on ? "on" : "off"));
    setflagProcess.start("/usr/sbin/parted -s " + device.toLocal8Bit() + " set " + flag.toLocal8Bit() + (on ? "on" : "off"));
    setflagProcess.waitForFinished(-1);
    updateDevTable();
    return setflagProcess.exitCode() == 0;
}

int Utils::getPartSize(QString device, QString fstype)
{
    if (fstype == "hfsplus")
        fstype = "hfs+"; /* ATV hack */
    QString command("/usr/sbin/parted -s " + device.toLocal8Bit() + " print | grep " + fstype + " | awk {'print $4'} | tr -d MB");
    QProcess partedProcess;
    partedProcess.start("/bin/sh -c \"" + command + "\"");
    partedProcess.waitForFinished(-1);
    int exitCode = partedProcess.exitCode();
    if (exitCode != 0)
        return -1;
    return QString(partedProcess.readAll()).toInt();
}

bool Utils::mkpart(QString device, QString fstype, QString start, QString end)
{
    logger->addLine("Calling mkpart for device: " + device + " and fs: " + fstype + " with start " + start + " and end " + end);
    QProcess partedProcess;
    partedProcess.start("/usr/sbin/parted -s " + device.toLocal8Bit() + " mkpart primary " + fstype + " " + start + " " + end);
    partedProcess.waitForFinished(-1);
    updateDevTable();
    return partedProcess.exitCode() == 0;
}

bool Utils::fmtpart(QString partition, QString fstype)
{
   logger->addLine("Calling fmtpart for partition " + partition + " and fstype " + fstype);
    QProcess mkfsProcess;
    if (fstype == "ext4")
    {
        mkfsProcess.start("/usr/sbin/mkfs.ext4 -F -I 256 -E stride=2,stripe-width=1024,nodiscard -b 4096 " + partition);
    }
    else if (fstype == "hfs+")
        mkfsProcess.start("/usr/sbin/mkfs.hfsplus " + partition);
    mkfsProcess.waitForFinished(-1);
    logger->addLine(QString(mkfsProcess.readAll()));
    return mkfsProcess.exitCode() == 0;
}

void Utils::writeToFile(QFile &file, QStringList strings, bool append)
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
    stream.flush();
    file.close();
}

bool Utils::mountPartition(Target *device, QString path)
{
    QDir pathdir;
    pathdir.mkpath(path);
    if (path == QString(MNT_BOOT))
    {
        logger->addLine("Trying to mount to MNT_BOOT ("+QString(MNT_BOOT) + ")");
        logger->addLine("Using device->boot: " + device->getBoot() + " and FS: " + device->getBootFS());
        QString bootFS = device->getBootFS();
        if (bootFS == "fat32") { bootFS = "vfat"; }
        if (this->getOSMCDev() != "atv")
            return (mount(device->getBoot().toLocal8Bit(), MNT_BOOT, bootFS.toLocal8Bit(), (device->isBootRW() == true) ? 0 : 1, "") == 0) ? true : false;
        else
        {
            QString mountCmd = "/bin/mount -t hfsplus -o force,rw " + device->getBoot() + " " + MNT_BOOT;
            return system(mountCmd.toLocal8Bit());
        }
    }
    else if (path == QString(MNT_ROOT))
    {
        logger->addLine("Trying to mount to MNT_ROOT ("+QString(MNT_ROOT) + ")");
        logger->addLine("Using device->root: " + device->getRoot());
        if (device->getRoot().contains(":/") && device->hasRootChanged())
        {
            logger->addLine("Assuming NFS mount.");
            /* This is an NFS share, use BusyBox */
            QProcess mountProcess;
            mountProcess.start("/bin/mount -t nfs -o nolock,noatime,vers=3 " + device->getRoot().toLocal8Bit() + " " +  MNT_ROOT);
            mountProcess.waitForFinished(-1);
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
    logger->addLine("Unsupported mountpoint.");
    return false;
}

bool Utils::unmountPartition(Target *device, QString path)
{
    if (path == QString(MNT_BOOT))
    {
        logger->addLine("Trying to unmount MNT_BOOT ("+QString(MNT_BOOT) + ")");
        logger->addLine("Using device->boot: " + device->getBoot());
        return (umount(device->getBoot().toLocal8Bit()) == 0) ? true : false;
    }
    if (path == QString(MNT_ROOT))
    {
        logger->addLine("Trying to unmount MNT_ROOT ("+QString(MNT_ROOT) + ")");
        logger->addLine("Using device->root: " + device->getRoot());
        return (umount(device->getRoot().toLocal8Bit()) == 0) ? true : false;
    }

}
