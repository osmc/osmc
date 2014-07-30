#include "utils.h"
#include <QString>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QTranslator>
#include <QApplication>

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

    void loadTranslation(QString locale, QApplication *application)
    {
        QTranslator translator;
        translator.load(QString("osmc_") + locale);
        application->installTranslator(&translator);
    }

}
