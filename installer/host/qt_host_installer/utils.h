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
    void displayError(QString title, QString message, bool isCritical = false);
    bool promptYesNo(QWidget *parent, QString title, QString question);
    int inline getBuildNumber() { return BUILD_NUMBER; }
    QList<SupportedDevice *> buildDeviceList();
    static int INSTALL_NOPRESEED = -1;
    static int INSTALL_SD = 1;
    static int INSTALL_USB = 2;
    static int INSTALL_NFS = 3;
    static int INSTALL_EMMC = 4;
    static int WIRELESS_ENCRYPTION_NONE = 0;
    static int WIRELESS_ENCRYPTION_WPAPSK = 1;
    static int WIRELESS_ENCRYPTION_WEP = 2;
}
