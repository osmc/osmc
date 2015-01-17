/*
 * (c) 2014-2015 Sam Nazarko
 * email@samnazarko.co.uk
*/
#include "logger.h"
#include <QDateTime>
#include <QFile>
#include "utils.h"
#include <QDebug>

Logger::Logger()
{
    /* Could add QSerialPort support one day */
}

void Logger::addLine(QString line)
{
    QDateTime timestamp = QDateTime::currentDateTime();
    qDebug() << timestamp.toString() << " " << line;
    log.append(timestamp.toString() + " " + line + "\n");
    fflush(stdout);
}
