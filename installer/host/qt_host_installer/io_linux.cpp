/*
 * (c) 2014-2015 Sam Nazarko
 * email@samnazarko.co.uk
*/
#include "io.h"
#include <QProcess>
#include <QStringList>
#include "utils.h"
#include "diskdevice.h"
#include <QFile>
#include "sys/mount.h"
#include <stdlib.h>
#include "writeimageworker.h"
#include <errno.h>

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
     process.setEnvironment(QStringList() << "LANG=C");
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
                 nd = addAdditionalInfo(nd);
                 if (nd->getIsWritable())
                     devices.append(nd);
             }
         }
     }
     return devices;
     }

bool writeImage(QString devicePath, QString deviceImage, QObject *caller)
{
    utils::writeLog("Writing " + deviceImage + " to " + devicePath);
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
    if(worker){
        worker->emitFlushingFS();
    }
    imageFile.close();
    if(!deviceFile.flush()) {
        return false;
    }
    deviceFile.close();

    updateKernelTable();
    return true;
}

bool mount(QString diskPath, QString mountDir)
{
    return mount(diskPath.toLocal8Bit(), mountDir.toLocal8Bit(), "vfat", 0, "") == 0;
}

bool unmount(QString devicePath, bool isDisk)
{
    utils::writeLog("Unmounting " + devicePath);
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
            utils::writeLog("Trying to unmount " + devicePartition.at(0) + " at " + devicePartition.at(1));
            int um = umount(devicePartition.at(1).toLocal8Bit());
            if (um == 0)
            {
                utils::writeLog("Partition unmounted successfully, continuing!");
            }
            else if (um == -1)
            {
                utils::writeLog("An error occured unmounting the partition. ERROR: "+QString::number(errno));
                ret = false;
            }
            else
            {
                utils::writeLog("An error occured unmounting the partition. retVal: "+QString::number(um));
                ret = false;
            }
        }
    }

    updateKernelTable();
    return ret;
}

void updateKernelTable()
{
    utils::writeLog("Running partprobe to inform operating system about partition table changes");
    system("/sbin/partprobe");
}

bool installImagingTool() { return true; }

DiskDevice* addAdditionalInfo(DiskDevice* diskDevice)
{
    QStringList entryPathSplit = diskDevice->getDiskPath().split("/");
    if (entryPathSplit[2].startsWith("mmcblk"))
    {
        /* Ugly hack: drivers/mmc/card/block.c considers all mmcblks unremovable, but
         * it seems that SD card readers can be registered as this */
        diskDevice->setIsWritable(true);
        return diskDevice;
    }
    QFile removeFile("/sys/block/" + entryPathSplit[2] + "/removable");
    if(!removeFile.open(QIODevice::ReadOnly)) {
        utils::writeLog("Can't open /sys/block/ " + entryPathSplit[2] + "/removable");
        diskDevice->setIsWritable(false);
        return diskDevice;
    }
    QString input = removeFile.readLine();
    if (input.startsWith("1"))
        diskDevice->setIsWritable(true);
    else
        diskDevice->setIsWritable(false);
    removeFile.close();
    return diskDevice;
}

}
