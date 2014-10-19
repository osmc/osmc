#include "utils.h"
#include "cmdlineparser.h"
#include <QString>
#include <cstdlib>

namespace utils
{
QString getOSMCDev()
{
    char osmcdev[10];
    #ifdef Q_WS_QWS
    get_cmdline_option("osmcdev=", osmcdev, sizeof(osmcdev));
    return (QString(osmcdev).simplified());
    #else
    /* Pretend to be Pi on testing system */
    return("rbp");
    #endif
}
void rebootSystem()
{
    /* Only reboot on real system */
    #ifdef Q_WS_QWS
    system("/bin/sh -c \"/bin/echo 1 > /proc/sys/kernel/sysrq\"");
    system("/bin/sh -c \"/bin/echo b > /proc/sysrq-trigger\"");
    #endif
}
}
