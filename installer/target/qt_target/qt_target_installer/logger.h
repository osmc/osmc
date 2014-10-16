#ifndef LOGGER_H
#define LOGGER_H

#include <QString>
#include <QStringList>

class Logger
{
public:
    Logger();
    void addLine(QString line);
private:
    QStringList *log;
};

#endif // LOGGER_H
