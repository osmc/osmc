#include <QProcess>
#include <QString>
#include <QStringList>
#include "utils.h"
#include "nixdiskdevice.h"
#include <QList>
#include <QTextStream>
#include <QDebug>

namespace io
{
   QList<NixDiskDevice * > enumerateDeviceLinux()
    {
       QList<NixDiskDevice *> devices;
        utils::writeLog("Enumerating imageable devices for Linux");
        QProcess process;
        QStringList lines;
        //process.start("/usr/bin/sudo", QStringList() << "/sbin/fdisk" << "-l", QIODevice::ReadWrite | QIODevice::Text); /* To run in Qt */
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

   QList<NixDiskDevice *> enumerateDeviceOSX()
   {
       QList<NixDiskDevice *> devices;
       utils::writeLog("Enumerating imageable devices for OSX");
       QProcess process;
       QStringList lines;
       process.start("/usr/sbin/diskutil", QStringList() << "list", QIODevice::ReadWrite | QIODevice::Text);
       if (! process.waitForFinished())
           utils::writeLog("Could not execute diskutil to enumerate devices");
       else
       {
           QTextStream stdoutStream(process.readAllStandardOutput());
           while (true)
           {
               QString line = stdoutStream.readLine();
               if (line.isNull())
                   break;
               else
               {
                   lines << line << "\n";
               }
           }
           for (int i = 0; i < lines.count(); i++)
           {
               QString line = lines.at(i);
               if (line.startsWith("/dev/disk"))
               {
                   QString devicePath = lines.at(i);
                   line = lines.at(i + 4);
                   QStringList deviceAttr = line.split(" "); // Bloody diskutil doesn't TAB \t properly
                   QString deviceSpace = deviceAttr.at((deviceAttr.count() - 5)) + deviceAttr.at((deviceAttr.count() - 4));
                   deviceSpace.remove("*");
                   NixDiskDevice *nd = new NixDiskDevice(i, devicePath, deviceSpace);
                   devices.append(nd);
               }
           }
       }
       return devices;
      }
}
