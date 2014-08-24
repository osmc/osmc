#include <QString>
#include <QString>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QTranslator>
#include <QApplication>
#define BUILD_NUMBER 001

namespace utils
{
    void writeLog(QString strLog);
    void displayError(QString title, QString message);
    int inline getBuildNumber() { return BUILD_NUMBER; }
}
