#include <QString>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QList>

namespace utils
{
    void writeLog(QString strLog);
    void makeMountDirectories();
    bool mountBootDirectory(QString device, QString format);
    bool hasFileSystem();
    bool hasPreseed();
}
