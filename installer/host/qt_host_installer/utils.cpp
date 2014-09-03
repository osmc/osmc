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

    void displayError(QString title, QString message)
    {
        QMessageBox *errorMessageBox = new QMessageBox();
        errorMessageBox->setWindowTitle(title);
        errorMessageBox->setText(message);
        errorMessageBox->setStandardButtons(QMessageBox::Ok);
        errorMessageBox->exec();
    }

    QList<SupportedDevice * > buildDeviceList()
    {
        utils::writeLog("Enumerating supported devices");
        QList<SupportedDevice * > devices;
        SupportedDevice *RBP = new SupportedDevice("Raspberry Pi", "RBP", true, true, true, false);
        devices.append(RBP);
        return devices;
    }

}
