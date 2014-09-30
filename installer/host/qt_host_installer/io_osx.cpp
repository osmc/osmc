#include <QProcess>
#include <QString>
#include <QStringList>
#include "utils.h"
#include "nixdiskdevice.h"
#include <QList>
#include <QTextStream>
#include "io.h"

namespace io
{
   QList<NixDiskDevice *> enumerateDevice()
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
               QString line = stdoutStream.readLine().simplified(); /* Remove trailing and leading ws */
               if (line.isNull())
                   break;
               /* The line holding the device is the only line always starting with 0: */
               else if (line.startsWith("0:"))
               {
                   lines << line;
               }
           }
           for (int i = 0; i < lines.count(); i++)
           {
               QString line = lines.at(i);
               QStringList deviceAttr = line.split(" ");

               /*
                * THE FOLLOWING LIST CHANGES IF THE DISK WAS NOT INITIALISED
                * In that case, <partition schema name> is missing and the
                * index for the following elements has to be adressed by n-1
                * content is now:
                * [0] 0:
                * [1] <partition scheme name>
                * [2] <total_size>
                * [3] <size_unit>
                * [4] device name (disk0, disk1, etc)
                */

               /* TODO when we refactor IO into platform specific implementations
                * provide definitions for the fields for easy arithmetic on the indices
                */
               QString deviceSpace;
               QString devicePath("/dev/");
               if (deviceAttr.at(1).startsWith("*"))
               {
                   /* partition schema name was missing - uninitialised disk */
                   deviceSpace = deviceAttr.at(1) + " " + deviceAttr.at(2);
                   devicePath += deviceAttr.at(3);
               } else
               {
                   deviceSpace = deviceAttr.at(2) + " " + deviceAttr.at(3);
                   devicePath += deviceAttr.at(4);
               }

               deviceSpace.remove("*");

               NixDiskDevice *nd = new NixDiskDevice(i, devicePath, deviceSpace);
               devices.append(nd);
           }
       }
       return devices;
      }

   bool writeImage(QString devicePath, QString deviceImage, QObject* caller)
   {
       QString aScript ="do shell script \"dd if="+ deviceImage + " of="+ devicePath +" bs=1m\" with administrator privileges";

       QString osascript = "/usr/bin/osascript";
       QStringList processArguments;
       processArguments << "-l" << "AppleScript";

       QProcess p;
       p.start(osascript);
       p.waitForStarted();
       p.write(aScript.toUtf8());
       p.closeWriteChannel();
       p.waitForReadyRead(-1);
       p.waitForFinished(-1);

       if (p.exitStatus() == QProcess::NormalExit) {
           utils::writeLog("Imaging was successful");
           return true;
       }
       else
       {
           utils::writeLog("Imaging failed!");
           return false;
       }
   }

   bool unmountDisk(QString devicePath)
   {
       QString aScript ="diskutil";

       QString osascript = "/usr/bin/osascript";
       QStringList processArguments;
       QProcess p;
       processArguments << "unmountDisk" << devicePath;

       p.start(aScript, processArguments);

       p.write(aScript.toUtf8());
       p.closeWriteChannel();
       p.waitForReadyRead(-1);
       p.waitForFinished(-1);
       QByteArray stderrArray = p.readAllStandardError();

       /* Non-empty stderr indicates failure */
       if (stderrArray.size() > 0)
       {
           utils::writeLog("Could not unmount " + devicePath + ". Message was: " + QString(stderrArray));
           return false;
       }
       else
          return true;
   }

   bool installImagingTool() { return true; }
}
