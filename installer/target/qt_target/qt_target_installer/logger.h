#ifndef LOGGER_H
#define LOGGER_H

#include <QString>
#include <QStringList>

class Logger
{
public:
    Logger();
    void addLine(QString line);
    QStringList *getLog() { return log; }
private:
    QStringList *log;
};

#endif // LOGGER_H
