#include "utils.h"
#include <QString>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QTranslator>
#include <QApplication>
#include <QMessageBox>
#include "supporteddevice.h"
#include <QList>
#include <QMessageBox>

namespace utils
{
    void writeLog(QString strLog)
    {
        QFile logFile("log.txt");
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


    bool promptYesNo(QWidget *parent, QString title, QString question)
    {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(parent, title, question, QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes)
            return true;
        else
            return false;
    }

    QList<SupportedDevice * > buildDeviceList()
    {
        utils::writeLog("Enumerating supported devices");
        QList<SupportedDevice * > devices;
        SupportedDevice *RBP = new SupportedDevice("Raspberry Pi", "RBP", true, true, true, false, true, false);
        devices.append(RBP);
        return devices;
    }

}
