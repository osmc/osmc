#include "io_linux.h"
#include <QProcess>
#include <QStringList>
#include "utils.h"
#include "nixdiskdevice.h"

namespace io
{
QList<NixDiskDevice * > enumerateDeviceLinux()
 {
    QList<NixDiskDevice *> devices;
     utils::writeLog("Enumerating imageable devices for Linux");
     QProcess process;
     QStringList lines;
     process.start("/usr/bin/sudo", QStringList() << "/sbin/fdisk" << "-l", QIODevice::ReadWrite | QIODevice::Text); /* To run in Qt */
     //process.start("/sbin/fdisk", QStringList() << "-l", QIODevice::ReadWrite | QIODevice::Text);
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
}
