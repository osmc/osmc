#include "utils.h"
#include <QString>
#include <QDateTime>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QDebug>

namespace utils
{
    void writeLog(QString strLog)
    {
        QFile logFile("/mnt/log.txt");
        QDateTime timestamp = QDateTime::currentDateTime();
        logFile.open(QIODevice::Append);
        if (logFile.isOpen())
        {
            QTextStream logStream(&logFile);
            logStream << timestamp.toString() << " " << strLog << '\n';
            logFile.close();
        }
        else
        {
            qDebug() << timestamp.toString() << " " << strLog;
        }
    }

    void makeMountDirectories()
    {
        system("mkdir -p /mnt/boot");
        system("mkdir -p /mnt/root");
    }

    bool mountBootDirectory(QString device, QString format)
    {
        QDir mountDirectory("/mnt/boot");
        if(mountDirectory.count() == 0)
        {
            utils::writeLog("Could not mount boot directory");
            return 0;
        }
        else
        {
            utils::writeLog("Boot directory was successfully mounted");
            return 1;
        }
    }

    bool hasFileSystem()
    {
        QFile filesystemFile("/mnt/boot/filesystem.tar.xz");
        if (filesystemFile.exists())
        {
            utils::writeLog("Filesystem archive found");
            return 1;
        }
        else
        {
            utils::writeLog("Filesystem archive was not found");
            return 0;
        }
    }

    bool hasPreseed()
    {
        QFile preseedFile("/mnt/boot/preseed.cfg");
        if (preseedFile.exists())
        {
            utils::writeLog("Preseeding file was found");
            return 1;
        }
        else
        {
            utils::writeLog("No preseeding file was found, using platform defaults instead");
            return 0;
        }
    }
}
