#include "utils.h"
#include <QFile>
#include <QDir>
#include <QLibrary>
#include "io.h"

namespace io
{
   bool installImagingTool()
    {
        utils::writeLog("Installing usbit32 imaging binary to temporary path");
        QFile::copy(":/assets/w32-lib/usbit32/usbitcmd.exe", QDir::temp().filePath("usbitcmd.exe"));
        if (QFile(QDir::temp().filePath("usbitcmd.exe")).exists())
        {
            utils::writeLog("Installation was successful");
            return true;
        }
        else
        {
            utils::writeLog("Installation was unsuccessful");
            return false;
        }
    }
}
