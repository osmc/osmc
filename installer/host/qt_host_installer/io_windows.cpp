/*
 * (c) 2014-2015 Sam Nazarko
 * email@samnazarko.co.uk
*/
#include "utils.h"
#include <QFile>
#include <QDir>
#include <QLibrary>
#include "io.h"
#include <QProcess>
#include <QStringList>
#include "diskdevice.h"
#include "writeimageworker.h"

namespace io
{
   inline void eraseMBR();

   QList<DiskDevice *> enumerateDevice()
   {
       QList<DiskDevice *> devices;
       utils::writeLog("Enumerating imageable devices for Windows");
       QProcess process;
       QStringList lines;
       process.start(QDir::temp().filePath("usbitcmd.exe"), QStringList() << "l" << "/d", QIODevice::ReadOnly | QIODevice::Text);
       if (! process.waitForFinished())
           utils::writeLog("Could not execute usbitcmd to enumerate devices");
       else
       {
           QTextStream stdoutStream(process.readAllStandardOutput());
           while (true)
           {
               QString line = stdoutStream.readLine();
               if (line.isNull())
                   break;
               else
                   lines << line;
           }

           /* Devices start on line 7; bottom 2 are padding */
           if (lines.count() > 9)
           {
           for (int i = 7; i < lines.count() - 2; i++)
           {
               QString line = lines.at(i);
               line = line.simplified();
               QStringList deviceAttr = line.split("|");
               int deviceID = deviceAttr.at(0).toInt();
               /* No drive letter and you've come to look at this? Don't. Unformatted disks have no mounted letter */
               QString devicePath = QString(deviceAttr.at(deviceAttr.count() - 2)).remove(" ");
               QString deviceSpace = QString(deviceAttr.at(deviceAttr.count() - 1)).remove(" ");
               DiskDevice *nd = new DiskDevice(deviceID, devicePath, deviceSpace);
               nd = addAdditionalInfo(nd);
               if (nd->getIsWritable())
                   devices.append(nd);

           }
           }
           else
               utils::writeLog("No attached devices detected");
           }
       return devices;
   }

   bool writeImage(QString devicePath, QString deviceImage, QObject* caller) /* We are really passing deviceID on Windows, but call it devicePath still */
   {
       utils::writeLog("Writing 512 bytes to erase MBR");
       QProcess process;
       process.start(QDir::temp().filePath("usbitcmd.exe"), QStringList() << "r" << devicePath << QDir::temp().filePath("bs.img") << "/d", QIODevice::ReadOnly | QIODevice::Text);
       process.waitForReadyRead(-1);
       process.waitForFinished(-1);
       QByteArray stdoutmbrArray = process.readAllStandardOutput();
       QByteArray stderrmbrArray = process.readAllStandardError();
       int exitCode = process.exitCode();
       if (exitCode != 0)
       {
           utils::writeLog("Could not trash MBR");
           utils::writeLog("Messages: stdout: " + QString(stdoutmbrArray));
           utils::writeLog("Messages: stderr: " + QString(stderrmbrArray));
           return false;
       }
       else
       {
           utils::writeLog("MBR is trashed. Now beginning imaging process.");
           WriteImageWorker* worker = NULL;
           if(caller) {
               if(! (worker = qobject_cast<WriteImageWorker*>(caller)) ) {
                   worker = NULL;
               }
           }
           process.start(QDir::temp().filePath("usbitcmd.exe"), QStringList() << "r" << devicePath << deviceImage << "/d", QIODevice::ReadOnly | QIODevice::Text);
           /* ToDo: read percentage and update progress; see if successful. We don't 'wait' for it to finish here but read line by line periodically. Check for .startsWith("Error") and write that to log */
           /* SN 05-10-14: can't get progress with QTextStream on the proc and even "/s" invocation. */
           process.waitForReadyRead(-1);
           process.waitForFinished(-1);
           QByteArray stdoutArray = process.readAllStandardOutput();
           QByteArray stderrArray = process.readAllStandardError();
           int exitCode = process.exitCode();

           if (exitCode != 0)
           {
               utils::writeLog("Could not invoke usbitcmd to write disk image");
               utils::writeLog("Messages: stdout: " + QString(stdoutArray));
               utils::writeLog("Messages: stderr: " + QString(stderrArray));
               return false;
           }

           else
           {
               return true;
           }
       }
   }

   bool installImagingTool()
    {
        utils::writeLog("Installing usbit32 imaging binary to temporary path");
        QFile::copy(":/assets/w32-lib/usbit32/usbitcmd.exe", QDir::temp().filePath("usbitcmd.exe"));
        /* Write empty boot sector for trashing MBR */
        utils::writeLog("Installing empty boot sector to temporary path");
        QFile::copy(":/assets/w32-lib/usbit32/bs.img", QDir::temp().filePath("bs.img"));
        if (QFile(QDir::temp().filePath("usbitcmd.exe")).exists())
        {
            utils::writeLog("Installation was successful");
            return true;
        }
        else
        {
            utils::writeLog("Installation was unsuccessful");
            return false;
        }
    }

   void updateKernelTable() { return; }

   DiskDevice* addAdditionalInfo(DiskDevice* diskDevice)
   {
       diskDevice->setIsWritable(true);
       return diskDevice;
   }
}
