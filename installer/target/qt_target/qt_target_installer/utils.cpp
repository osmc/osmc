#include "utils.h"
#include "cmdlineparser.h"
#include <QString>

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

    #endif
}
}
