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
#if defined(Q_OS_LINUX)
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
#endif
#if defined(Q_OS_MAC)
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
                * THE FOLLOWING LIST CHANGES IF THE DISK WAS NOT INITIALISED
                * In that case, <partition schema name> is missing and the
                * index for the following elements has to be adressed by n-1
                * content is now:
                * [0] 0:
                * [1] <partition scheme name>
                * [2] <total_size>
                * [3] <size_unit>
                * [4] device name (disk0, disk1, etc)
                **/

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

   bool writeImageOSX(QString devicePath, QString deviceImage)
   {
       //MAYBE THIS WORKS WHEN IN THE WIDGET?
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
       QByteArray stdout = p.readAllStandardOutput();
       QByteArray stderr = p.readAllStandardError();

       qDebug() << "the stdout of the script is" << QString(stdout);
       qDebug() << "the stderr of the script is" << QString(stderr);
       qDebug() << "exitCode: " << p.exitCode();

       if (p.exitStatus() == QProcess::NormalExit) {
           qDebug("normal exit");
       } else {
           qDebug("crash exit");
       }

       return true;
   }

   bool unmountDiskOSX(QString devicePath)
   {
       //QString aScript ="do shell script \"diskutil unmountDisk "+ devicePath +"\" with administrator privileges";
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

       /* assume that a non empty stdErr means everything failed */
       if (stderrArray.size() > 0)
       {
           utils::writeLog("Could not unmount " + devicePath + ". Message was: " + QString(stderrArray));
           return false;
       } else
       {
          /* hooray! */
          return true;
       }
   }
#endif /* endif OSX */

   /*!
       Read the last four bytes of the given file and interpret them
       to be the size in bytes of the uncompressed gzip file.

       This method does not test if the given filename denotes
       an actual gzip file.

       This method does not care about endianess.

       Return the size in bytes.
       Returns -1 if the file could not be opened.
   */
   int getDecompressedSize(QString gzFilename)
   {
       /* size of the uncompressed file can be found
        * in the last four bytes. It doesn't seem to be too exact
        * http://www.abeel.be/content/determine-uncompressed-size-gzip-file
        */
       QFile sourceFile(gzFilename);
       bool sourceopen = sourceFile.open(QIODevice::ReadOnly);
       if (!sourceopen)
       {
           utils::writeLog("io:getDecompressedSize: Could not open file "
                           + gzFilename
                           + " to determine decompressed size");
           return -1;
       }

       QByteArray buffer = sourceFile.readAll();
       int b4 = buffer.at(buffer.size()-4);
       int b3 = buffer.at(buffer.size()-3);
       int b2 = buffer.at(buffer.size()-2);
       int b1 = buffer.at(buffer.size()-1);
       int val = (b1 << 24) | (b2 << 16) + (b3 << 8) + b4;
       sourceFile.close();

       return val;
   }
}
