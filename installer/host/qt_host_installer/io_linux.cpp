#include "io_linux.h"
#include <QProcess>
#include <QStringList>
#include "utils.h"
#include "nixdiskdevice.h"
#include <QFile>
#if defined(Q_OS_LINUX)
#include "sys/mount.h"
#include <stdlib.h>
#endif

namespace io
{
QList<NixDiskDevice * > enumerateDeviceLinux()
 {
    UpdateKernelTable();
    QList<NixDiskDevice *> devices;
     utils::writeLog("Enumerating imageable devices for Linux");
     QProcess process;
     QStringList lines;
     //process.start("/usr/bin/gksudo", QStringList() << "/sbin/fdisk -l", QIODevice::ReadWrite | QIODevice::Text); /* To run in Qt */
     process.start("/sbin/fdisk", QStringList() << "-l", QIODevice::ReadWrite | QIODevice::Text);
     if (! process.waitForFinished())
         utils::writeLog("Could not execute fdisk to enumerate devices");
     else
     {
         QTextStream stdoutStream(process.readAllStandardOutput());
         while (true)
         {
             QString line = stdoutStream.readLine();
             if (line.isNull())
                 break;
             else
                 lines << line << "\n";
         }
         for (int i = 0; i < lines.count(); i++)
         {
             QString line = lines.at(i);
             if (line.startsWith("Disk /dev"))
             {
                 QStringList deviceAttr = line.split(" ");
                 QString devicePath;
                 QString deviceSpace;
                 devicePath = deviceAttr.at(1);
                 devicePath.remove(":");
                 deviceSpace = deviceAttr.at(2) + deviceAttr.at(3);
                 deviceSpace.remove(",");
                 NixDiskDevice *nd = new NixDiskDevice(i, devicePath, deviceSpace);
                 devices.append(nd);
             }
         }
     }
     return devices;
     }
bool writeImageLinux(QString devicePath, QString deviceImage)
{
    UpdateKernelTable();
}

bool unmountDiskLinux(QString devicePath)
{
    /* Read /proc/mounts and find out what partitions of the disk we are using are mounted */
    QFile partitionsFile("/proc/mounts");
    if(!partitionsFile.open(QIODevice::ReadOnly)) {
        utils::writeLog("Can't read /proc/mounts!");
        return false;
    }

    QString input(partitionsFile.readAll());
    partitionsFile.close();

    QStringList lines = input.split("\n");
    bool ret = true;

    QListIterator<QString> i(lines);
    while (i.hasNext()) {
        QString line = i.next();
        if (line.startsWith(devicePath))
        {
            QStringList devicePartition = line.split(" ");
            utils::writeLog("Trying to unmount " + devicePartition.at(0));
            #if defined(Q_OS_LINUX)
            if (umount(devicePartition.at(0).toLocal8Bit()))
            {
                utils::writeLog("Partition unmounted successfully, continuing!");
            }
            else
            {
                utils::writeLog("An error occured unmounting the partition");
                ret = false;
            }
           #endif
                return false;
        }
    }

    UpdateKernelTable();
    return ret;
}

void UpdateKernelTable()
{
    #if defined(Q_OS_LINUX)
    utils::writeLog("Running partprobe to inform operating system about partition table changes");
    system("/sbin/partprobe");
    #endif
}

}
