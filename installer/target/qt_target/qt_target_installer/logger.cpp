#include "logger.h"
#include <QDateTime>
#include <QFile>
#include "utils.h"

Logger::Logger()
{
    log = new QStringList();
    /* Could add QSerialPort support one day */
}

void Logger::addLine(QString line)
{
    QDateTime timestamp = QDateTime::currentDateTime();
    log->append(timestamp.toString() + " " + line + "\n");
}
