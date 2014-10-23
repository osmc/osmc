#include "io.h"
#include <QProcess>
#include <QStringList>
#include "utils.h"
#include "diskdevice.h"
#include <QFile>
#include "sys/mount.h"
#include <stdlib.h>
#include "writeimageworker.h"

namespace io
{
inline void UpdateKernelTable();

QList<DiskDevice * > enumerateDevice()
 {
    updateKernelTable();
    QList<DiskDevice *> devices;
     utils::writeLog("Enumerating imageable devices for Linux");
     QProcess process;
     QStringList lines;
     //process.start("/usr/bin/gksudo", QStringList() << "/sbin/fdisk -l", QIODevice::ReadWrite | QIODevice::Text); /* To run in Qt */
     process.start("/sbin/fdisk", QStringList() << "-l", QIODevice::ReadOnly | QIODevice::Text);
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
                 DiskDevice *nd = new DiskDevice(i, devicePath, deviceSpace);
                 devices.append(nd);
             }
         }
     }
     return devices;
     }

bool writeImage(QString devicePath, QString deviceImage, QObject *caller)
{
    WriteImageWorker* worker = NULL;
    if(caller) {
        if(! (worker = qobject_cast<WriteImageWorker*>(caller)) ) {
            worker = NULL;
        }
    }
    QFile imageFile(deviceImage);
    QFile deviceFile(devicePath);
    bool imageOpen = imageFile.open(QIODevice::ReadOnly);
    bool deviceOpen = deviceFile.open(QIODevice::WriteOnly);
    if(!imageOpen) {
        utils::writeLog("Error opening image");
        return false;
    }
    if(!deviceOpen) {
        utils::writeLog("Error opening device");
        return false;
    }
    char buf[512*1024];
    QDataStream in(&imageFile);
    QDataStream out(&deviceFile);
    unsigned total = 0;
    int r = in.readRawData(buf,sizeof(buf));
    int ret;
    while(r>0) {
        ret = out.writeRawData(buf, r);
        if(ret == -1 || ret != r) {
            imageFile.close();
            deviceFile.close();
            utils::writeLog("Error writing to device");
            return false;
        }
        total += r;
        if(worker){
            worker->emitProgressUpdate(total);
        }
        r = in.readRawData(buf,sizeof(buf));
    }
    imageFile.close();
    deviceFile.close();

    updateKernelTable();
    return true;
}

bool mount(QString diskPath, QString mountDir)
{
    return mount(diskPath.toLocal8Bit(), mountDir.toLocal8Bit(), "vfat", 1, "");
}

bool unmount(QString devicePath, bool isDisk)
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
            if (umount(devicePartition.at(0).toLocal8Bit()))
            {
                utils::writeLog("Partition unmounted successfully, continuing!");
            }
            else
            {
                utils::writeLog("An error occured unmounting the partition");
                ret = false;
            }
        }
    }

    updateKernelTable();
    return ret;
}

void updateKernelTable()
{
    #if defined(Q_OS_LINUX)
    utils::writeLog("Running partprobe to inform operating system about partition table changes");
    system("/sbin/partprobe");
    #endif
}

bool installImagingTool() { return true; }

}
