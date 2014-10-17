#include "logger.h"
#include <QDateTime>
#include <QFile>
#include "utils.h"
#include <QDebug>

Logger::Logger()
{
    log = new QStringList();
    /* Could add QSerialPort support one day */
}

void Logger::addLine(QString line)
{
    QDateTime timestamp = QDateTime::currentDateTime();
    #ifdef QT_DEBUG
    qDebug() << line;
    #endif
    log->append(timestamp.toString() + " " + line + "\n");
}
