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
               utils::writeLog("=================================================");
               utils::writeLog("Starting to parse " + devicePath + " for additional info\n");
               nd = addAdditionalInfo(nd);

               if (nd->getIsWritable())
               {
                   utils::writeLog("Parsed device as writable. Appending.");
                   devices.append(nd);
               }
               else
               {
                   utils::writeLog("Parsed device as NON-writable. NOT Appending.");
               }
               utils::writeLog("\n");
               utils::writeLog("Finished parsing additional info for " + devicePath);
               utils::writeLog("=================================================\n");
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

   QString mount(QString devicePath, QString mountDir)
   {
       QString aScript ="diskutil";

       QStringList processArguments;
       QProcess p;
       processArguments << "mount" << devicePath;

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
                           + devicePath + " to " + mountDir + ". Messages are: stdErr: " + QString(stderrArray)
                           + "\n stdOut: " + QString(stdoutArray));
           return NULL;
       }
       else {
          // Since Sierra 10.12.5, the previous approach with giving the exact mountpoint
          // no longer works - no idea why.
          // we now mount to the default mountpoint and try to extract it from the actual
          // console output - this is really fishy
          QString stdoutString  = QString(stdoutArray);
          QStringList stdList = stdoutString.split("Volume ");
          QString volume = stdList.at(1).split(" on ").at(0);
          QString actualMountPoint = QString("/Volumes/" + volume);
          utils::writeLog("mounted " + devicePath + " with message " + stdoutString);
          utils::writeLog("Assuming mountpoint: " + actualMountPoint);
          return actualMountPoint;
       }
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
           utils::writeLog("Could not unmount " + devicePath + ". Messages are: stdErr: " + QString(stderrArray) + "\n stdOut: " + QString(stdoutArray));
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
                   utils::writeLog("Ejectable-Line: " + line);

                   if (0 == QString("Yes").compare(ejectable, Qt::CaseInsensitive)
                       || 0 == QString("Removable").compare(ejectable, Qt::CaseInsensitive))
                       isEjectable = true;

                   utils::writeLog("Determined " + ejectable + " as ejactableProperty for " + diskPath);
               }
               else if (line.simplified().startsWith("Protocol:"))
               {
                   utils::writeLog("Protocol-Line: " + line);
                   QString protocol = QString(line.split(":").at(1).simplified());
                   if (0 == QString("Disk Image").compare(protocol, Qt::CaseInsensitive))
                       isDmg = true;
                   utils::writeLog("Determined " + protocol + " as protocol for " + diskPath);
                   utils::writeLog(QString("Decided to be a DMG: ").append(isDmg ? "yes" : "no"));
               }
               else if (line.simplified().startsWith("Read-Only Media:"))
               {
                   utils::writeLog("R/O-Line: " + line);
                   QString readOnlyMedia = QString(line.split(":").at(1).simplified());
                   utils::writeLog("parsed/split/simplified readOnly line would have been: " + readOnlyMedia);

                   if (0 == QString("Yes").compare(readOnlyMedia, Qt::CaseInsensitive))
                       isReadOnly = true;

                   utils::writeLog("Determined " + readOnlyMedia + " as readOnlyMedia for " + diskPath);
                   utils::writeLog(QString("Decided to be r/o: ").append(isReadOnly ? "yes" : "no"));
               }
               else if (line.simplified().contains("Media Name"))
               {
                   utils::writeLog("MediaName-Line: " + line);
                   QString label = QString(line.split(":").at(1).simplified());
                   diskDevice->setLabel(label);
               }
               /* advance to next line */
               line = stdoutStream.readLine();
           }
           if (isEjectable && (!isDmg && !isReadOnly)) {
              utils::writeLog(QString("isEjectable: ").append(isEjectable ? "yes" : "no").append("; isDmg: ").append(isDmg ? "yes" : "no").append("; isReadOnly: ").append(isReadOnly ? "yes" : "no"));
              diskDevice->setIsWritable(true);
           }
           else
               utils::writeLog("Decided that " + diskPath + " is not writable to us");
       }

       return diskDevice;
   }
}
