/*
 * (c) 2014-2015 Sam Nazarko
 * email@samnazarko.co.uk
*/
#include <QProcess>
#include <QString>
#include <QStringList>
#include "utils.h"
#include "diskdevice.h"
#include <QList>
#include <QTextStream>
#include "io.h"

namespace io
{
   QList<DiskDevice *> enumerateDevice()
   {
       QList<DiskDevice *> devices;
       utils::writeLog("Enumerating imageable devices for OSX");
       QProcess process;
       QStringList lines;
       process.setEnvironment(QStringList() << "LANG=C");
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
               QString deviceSpace;
               QString devicePath("/dev/");
               if (deviceAttr.at(1).startsWith("*"))
               {
                   /* partition schema name was missing - uninitialised disk */
                   deviceSpace = deviceAttr.at(1) + " " + deviceAttr.at(2);
                   devicePath += (new QString(deviceAttr.at(3)))->replace(0, 1, "rd");
               } else
               {
                   deviceSpace = deviceAttr.at(2) + " " + deviceAttr.at(3);
                   QString deviceName(deviceAttr.at(4));
                   /* make the disk become a rdisk */
                   devicePath += deviceName.replace(0, 1, "rd");
               }

               deviceSpace.remove("*");
               DiskDevice *nd = new DiskDevice(i, devicePath, deviceSpace);
               nd = addAdditionalInfo(nd);

               if (nd->getIsWritable())
                   devices.append(nd);
           }
       }
       return devices;
   }

   bool writeImage(QString devicePath, QString deviceImage, QObject* caller)
   {
       devicePath.replace(" ", "\\ ");
       deviceImage.replace(" ", "\\ ");
       QString aScript ="do shell script \"dd if="+ deviceImage + " of="+ devicePath +" bs=1m conv=sync && sync\" with administrator privileges";

       QString osascript = "/usr/bin/osascript";
       QStringList processArguments;
       processArguments << "-l" << "AppleScript";

       QProcess p;
       p.setEnvironment(QStringList() << "LANG=C");
       utils::writeLog("going to start osa");
       p.start(osascript);
       p.waitForStarted();
       utils::writeLog("pasting admin script to process " + aScript);
       p.write(aScript.toUtf8());
       p.closeWriteChannel();
       utils::writeLog("waiting for finish");
       p.waitForReadyRead(-1);
       p.waitForFinished(-1);
       utils::writeLog("osa claims to be done...collect output and verify");
       QByteArray stdoutArray = p.readAllStandardOutput();
       QByteArray stderrArray = p.readAllStandardError();
       int exitCode = p.exitCode();

       if (exitCode == 0 && p.exitStatus() == QProcess::NormalExit) {
           utils::writeLog("Imaging was successful");
           return true;
       }
       else
       {
           utils::writeLog("Imaging failed!");
           utils::writeLog("Messages are:");
           utils::writeLog("\t stdout: " + QString(stdoutArray));
           utils::writeLog("\t stderr: " + QString(stderrArray));
           return false;
       }
   }

   bool mount(QString devicePath, QString mountDir)
   {
       QString aScript ="diskutil";

       QStringList processArguments;
       QProcess p;
       processArguments << "mount" << "-mountPoint" << mountDir << devicePath;

       p.start(aScript, processArguments);

       p.closeWriteChannel();
       p.waitForReadyRead(-1);
       p.waitForFinished(-1);
       QByteArray stdoutArray = p.readAllStandardOutput();
       QByteArray stderrArray = p.readAllStandardError();
       int exitCode = p.exitCode();

       /* Non-0 exit code indicates failure */
       if (exitCode != 0)
       {
           utils::writeLog("Could not mount "
                           + devicePath + ". Messages are: stdErr: " + QString(stderrArray)
                           + "\n stdOut: " + QString(stdoutArray));
           return false;
       }
       else
          return true;
   }

   bool unmount(QString devicePath, bool isDisk)
   {
       QString aScript ="diskutil";

       QStringList processArguments;
       QProcess p;
       if (isDisk)
           processArguments << "unmountDisk";
       else
           processArguments << "unmount";

       processArguments << devicePath;

       p.start(aScript, processArguments);

       p.write(aScript.toUtf8());
       p.closeWriteChannel();
       p.waitForReadyRead(-1);
       p.waitForFinished(-1);
       QByteArray stdoutArray = p.readAllStandardOutput();
       QByteArray stderrArray = p.readAllStandardError();
       int exitCode = p.exitCode();

       /* Non-0 exit code indicates failure */
       if (exitCode != 0)
       {
           utils::writeLog("Could not mount " + devicePath + ". Messages are: stdErr: " + QString(stderrArray) + "\n stdOut: " + QString(stdoutArray));
           return false;
       }
       else
          return true;
   }

   bool installImagingTool() { return true; }
   void updateKernelTable() { return; }

   DiskDevice* addAdditionalInfo(DiskDevice* diskDevice)
   {
       QProcess process;
       QString diskPath = diskDevice->getDiskPath();
       process.start("/usr/sbin/diskutil", QStringList() << "info" << diskPath, QIODevice::ReadWrite | QIODevice::Text);
       if (! process.waitForFinished())
           utils::writeLog("Could not execute diskutil to check protocol for device " + diskPath);
       else
       {
           QTextStream stdoutStream(process.readAllStandardOutput());
           QString line = stdoutStream.readLine();
           bool isEjectable = false;
           bool isDmg = false;
           bool isReadOnly = false;
           while (!line.isNull())
           {
               line = line.simplified(); /* Remove trailing and leading ws */
               /* The line holding the device is the only line always starting with 0: */
               if (line.simplified().startsWith("Ejectable:") || line.simplified().startsWith("Removable Media:"))
               {
                   QString ejectable = QString(line.split(":").at(1).simplified());
                   if (0 == QString("Yes").compare(ejectable, Qt::CaseInsensitive))
                       isEjectable = true;
                   utils::writeLog("Determined " + ejectable + " as ejactableProperty for " + diskPath);
               }
               else if (line.simplified().startsWith("Protocol:"))
               {
                   QString protocol = QString(line.split(":").at(1).simplified());
                   if (0 == QString("Disk Image").compare(protocol, Qt::CaseInsensitive))
                       isDmg = true;
                   utils::writeLog("Determined " + protocol + " as protocol for " + diskPath);
                   utils::writeLog("Decided to be a DMG: " + isDmg ? "yes" : "no");
               }
               else if (line.simplified().startsWith("Read-Only Media:"))
               {
                   QString readOnlyMedia = QString(line.split(":").at(1).simplified());
                   if (0 == QString("Yes").compare(readOnlyMedia, Qt::CaseInsensitive))
                       isReadOnly = true;
                   utils::writeLog("Determined " + readOnlyMedia + " as readOnlyMedia for " + diskPath);
                   utils::writeLog("Decided to be r/o: " + isReadOnly ? "yes" : "no");
               }
               else if (line.simplified().contains("Media Name"))
               {
                   QString label = QString(line.split(":").at(1).simplified());
                   diskDevice->setLabel(label);
               }
               if (isEjectable && (!isDmg && !isReadOnly))
                  diskDevice->setIsWritable(true);
               else
                   utils::writeLog("Decided that " + diskPath + " is not writable to us");

               /* advance to next line */
               line = stdoutStream.readLine();
           }
       }
       return diskDevice;
   }
}
