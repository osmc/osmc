/*
 * (c) 2014-2015 Sam Nazarko
 * email@samnazarko.co.uk
*/
#include "utils.h"
#include <QString>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDebug>
#include <QTranslator>
#include <QApplication>
#include <QMessageBox>
#include "supporteddevice.h"
#include <QList>
#include <QDir>
#include <QMessageBox>
#ifdef Q_OS_LINUX
#include <unistd.h>
#endif

namespace utils
{
    void writeLog(QString strLog)
    {
        QFile logFile(QDir::homePath() + "/" + "osmc_installer_log.txt");
        QDateTime timestamp = QDateTime::currentDateTime();
        logFile.open(QIODevice::Append);
        if (logFile.isOpen())
        {
#ifdef Q_OS_LINUX
            // Set the owner and group the same as the home path
            QFileInfo info(QDir::homePath());
            fchown(logFile.handle(),info.ownerId(),info.groupId());
#endif
            QTextStream logStream(&logFile);
            logStream << timestamp.toString() << " " << strLog << '\n';
            logFile.close();
        }
        else
        {
            qDebug() << timestamp.toString() << " " << strLog;
        }
    }

    void displayError(QString title, QString message, bool isCritical)
    {
        QMessageBox *errorMessageBox = new QMessageBox();
        errorMessageBox->setWindowTitle(title);
        errorMessageBox->setText(message);
        errorMessageBox->setStandardButtons(QMessageBox::Ok);
        if (isCritical)
        {
            errorMessageBox->setIcon(QMessageBox::Critical);
        }
        errorMessageBox->exec();
    }


    bool promptYesNo(QString title, QString question)
    {
        QMessageBox *questionBox = new QMessageBox();
        questionBox->setWindowTitle(title);
        questionBox->setText(question);
        questionBox->setStandardButtons(QMessageBox::Yes | QMessageBox::No);

        if (questionBox->exec() == QMessageBox::Yes)
            return true;
        else
            return false;
    }

    QList<SupportedDevice * > buildDeviceList()
    {
        utils::writeLog("Enumerating supported devices");
        QList<SupportedDevice * > devices;
        SupportedDevice *VERO = new SupportedDevice("Vero", "VERO1", true, false, false, false, true, false);
        SupportedDevice *RBP = new SupportedDevice("Raspberry Pi 1 / Zero", "RBP", true, true, true, false, true, false);
        SupportedDevice *RBP2 = new SupportedDevice("Raspberry Pi 2", "RBP2", true, true, true, false, true, false);
        SupportedDevice *ATV = new SupportedDevice("Apple TV", "appletv", false, false, false, false, false, false);
        devices.append(VERO);
        devices.append(RBP);
        devices.append(RBP2);
        devices.append(ATV);
        return devices;
    }

    bool validateIp(QString ip)
    {
        if (ip.isNull() || ip.isEmpty())
            return false;

        QStringList elements = ip.split(".");
        for (int i = 0; i < elements.count(); i++)
        {
            QString element = elements.at(i);
            if (element.isEmpty() || element.toInt() < 0 || element.toInt() > 255)
                return false;
        }

        return true;
    }

}
