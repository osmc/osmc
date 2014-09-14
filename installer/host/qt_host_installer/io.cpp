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
               QString line = stdoutStream.readLine().simplified(); //remove trailing and leading ws
               if (line.isNull())
                   break;
               // the line holding the device is the only line always starting with 0:
               else if (line.startsWith("0:"))
               {
                   lines << line;
               }
           }
           for (int i = 0; i < lines.count(); i++)
           {
               QString line = lines.at(i);
               QStringList deviceAttr = line.split(" ");
               /**
                * content is now:
                * [0] 0:
                * [1] <partition scheme name>
                * [2] <total_size>
                * [3] <size_unit>
                * [4] device name (disk0, disk1, etc)
                **/
               QString deviceSpace = deviceAttr.at(2) + " " + deviceAttr.at(3);
               deviceSpace.remove("*");
               QString devicePath = "/dev/" + deviceAttr.at(4);
               NixDiskDevice *nd = new NixDiskDevice(i, devicePath, deviceSpace);
               devices.append(nd);
           }
       }
       return devices;
      }

   /* TODO - not sure what has to be done here to make this concurrent so we can have a wait-spin-icon
    * on the GUI
    */
   bool writeImageOSX(QString devicePath, QString deviceImage)
   {
       QString aScript ="do shell script \"dd if="+ deviceImage + " of="+ devicePath +" bs=1m\" with administrator privileges";

       QString osascript = "/usr/bin/osascript";
       QStringList processArguments;
       processArguments << "-l" << "AppleScript";

       QProcess p;
       p.start(osascript, processArguments);
       p.write(aScript.toUtf8());
       p.closeWriteChannel();
       p.waitForReadyRead(-1);
       QByteArray result = p.readAll();
       QString resultAsString(result); // if appropriate
       qDebug() << "the result of the script is" << resultAsString;
       p.waitForFinished(-1);

       return true;
   }
}
