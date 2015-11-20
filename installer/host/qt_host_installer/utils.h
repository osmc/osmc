/*
 * (c) 2014-2015 Sam Nazarko
 * email@samnazarko.co.uk
*/
#include <QString>
#include <QString>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QApplication>
#include "supporteddevice.h"
#include <QList>

#define BUILD_NUMBER 121

namespace utils
{
    void writeLog(QString strLog);
    void displayError(QString title, QString message, bool isCritical = false);
    bool promptYesNo(QString title, QString question);
    bool validateIp(QString ip);
    int inline getBuildNumber() { return BUILD_NUMBER; }
    QList<SupportedDevice *> buildDeviceList();
    const static int INSTALL_NOPRESEED = -1;
    const static int INSTALL_SD = 1;
    const static int INSTALL_USB = 2;
    const static int INSTALL_NFS = 3;
    const static int INSTALL_EMMC = 4;
    const static int INSTALL_PARTITIONER = 5;
    const static int WIRELESS_ENCRYPTION_NONE = 0;
    const static int WIRELESS_ENCRYPTION_WPAPSK = 1;
    const static int WIRELESS_ENCRYPTION_WEP = 2;
}
