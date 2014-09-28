#include <QString>
#if defined(Q_OS_LINUX)
#include "io_linux.h"
#endif
#if defined(Q_OS_MAC)
#include "io_osx.h"
#endif
#if defined(Q_OS_WIN32) || defined(Q_OS_WIN)
#include "io_windows.h"
#endif

namespace io
{
     int getDecompressedSize(QString gzFilename);
     qint64 getFileSize(QString filename);
}
