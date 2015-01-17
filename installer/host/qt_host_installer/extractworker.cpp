/*
 * (c) 2014-2015 Sam Nazarko
 * email@samnazarko.co.uk
*/
#include "extractworker.h"
#include "zlib.h"
#include "assert.h"
#include <QString>
#include "stdio.h"
#include <QFile>
#include "utils.h"
#include <QDir>
#ifdef Q_OS_LINUX
#include <unistd.h>
#endif

#define CHUNKSIZE 32768

ExtractWorker::ExtractWorker(QString sourcename, QString targetname)
{
    this->sourceName = QString(sourcename);
    this->destName = QString(targetname);
}

void ExtractWorker::process()
{
    QFile sourceFile(sourceName);
    QFile targetFile(destName);
    QString home = QDir::homePath();
    bool sourceopen = sourceFile.open(QIODevice::ReadOnly);
    if (!sourceopen)
    {
        utils::writeLog("Could not open sourcefile " + sourceName);
        utils::writeLog("Errorstring was " + sourceFile.errorString());
        emit error();
        return;
    }
    int sourceFileDescriptor = sourceFile.handle();
    FILE* source = fdopen(sourceFileDescriptor, "rb");

    bool targetopen = targetFile.open(QIODevice::WriteOnly);
    if (! targetopen)
    {
        utils::writeLog("Could not open target file " + destName);
        utils::writeLog("Error string was " + targetFile.errorString());
        emit error();
        return;
    }
    int targetFileDescriptor = targetFile.handle();
    FILE* dest = fdopen(targetFileDescriptor, "wb");
#ifdef Q_OS_LINUX
    // Set the owner and group the same as the home path
    QFileInfo info(QDir::homePath());
    fchown(fileno(dest),info.ownerId(),info.groupId());
#endif

    int ret;
    unsigned have;
    unsigned written = 0;
    z_stream strm;
    unsigned char in[CHUNKSIZE];
    unsigned char out[CHUNKSIZE];
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    ret = inflateInit2(&strm,  47);
    if (ret != Z_OK)
    {
        emit error();
        return;
    }

    do
    {
        strm.avail_in = fread(in, 1, CHUNKSIZE, source);

        if (ferror(source))
        {
            (void)inflateEnd(&strm);
            emit error();
            return;
        }
        if (strm.avail_in == 0)
            break;
        strm.next_in = in;

        do
        {
            strm.avail_out = CHUNKSIZE;
            strm.next_out = out;
            ret = inflate(&strm, Z_NO_FLUSH);
            assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
            switch (ret) {
            case Z_NEED_DICT:
                ret = Z_DATA_ERROR;     /* and fall through */
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                (void)inflateEnd(&strm);
                emit error();
                return;
         }
            have = CHUNKSIZE - strm.avail_out;
            written += have;

            emit progressUpdate(written);
            if (fwrite(out, 1, have, dest) != have || ferror(dest))
            {
                (void)inflateEnd(&strm);
                emit error();
                return;
            }
         }
        while (strm.avail_out == 0);
    }
    while (ret != Z_STREAM_END);

    (void)inflateEnd(&strm);
    if (ret == Z_STREAM_END)
        emit finished();
    else
        emit error();
}
