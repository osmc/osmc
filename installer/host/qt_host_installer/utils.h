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
    static int INSTALL_NOPRESEED = -1;
    static int INSTALL_SD = 1;
    static int INSTALL_USB = 2;
    static int INSTALL_NFS = 3;
    static int INSTALL_EMMC = 4;
}
