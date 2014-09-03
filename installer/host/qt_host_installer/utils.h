#include <QString>
#include <QString>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QApplication>
#include "supporteddevice.h"
#include <QList>

#define BUILD_NUMBER 001

namespace utils
{
    void writeLog(QString strLog);
    void displayError(QString title, QString message);
    int inline getBuildNumber() { return BUILD_NUMBER; }
    QList<SupportedDevice *> buildDeviceList();
}
