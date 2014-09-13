#include "extractprogress.h"
#include "ui_extractprogress.h"
#include <QString>
#include <QByteArray>
#include "zlib.h"
#include "utils.h"
#include "assert.h"
#define CHUNKSIZE 8192

ExtractProgress::ExtractProgress(QWidget *parent, QString devicePath, QString deviceImage):
    QWidget(parent),
    ui(new Ui::ExtractProgress)
{
    ui->setupUi(this);
    bool extractSuccess = true; /* True as we only need to change if failed */
    if (deviceImage.contains(".gz"))
        extractSuccess = doExtraction(deviceImage);
    if (extractSuccess)
    {
        /* Write the image to the block device */

    }
    /* Peform pre-seeding operations and final configuration */
}

bool ExtractProgress::doExtraction(QString deviceImage)
{
    /* Based off http://www.zlib.net/zpipe.c */
    utils::writeLog("Extracting " + deviceImage);
    int ret;
    unsigned have;
    z_stream strm;
    unsigned char in[CHUNKSIZE];
    unsigned char out[CHUNKSIZE];
    QByteArray strByteArray = deviceImage.toLocal8Bit();
    FILE *source = fopen(strByteArray.data(), "w+");
    strByteArray = deviceImage.remove(".gz").toLocal8Bit();
    FILE *dest = fopen(strByteArray.data(), "w+");
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    ret = inflateInit(&strm);
    if (ret != Z_OK)
        return ret;

    do
    {
        strm.avail_in = fread(in, 1, CHUNKSIZE, source);
        if (ferror(source))
        {
            (void)inflateEnd(&strm);
            return Z_ERRNO;
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
                return ret;
         }
            have = CHUNKSIZE - strm.avail_out;
            if (fwrite(out, 1, have, dest) != have || ferror(dest))
            {
                (void)inflateEnd(&strm);
                return Z_ERRNO;
            }
         }
        while (strm.avail_out == 0);
    }
    while (ret != Z_STREAM_END);

    (void)inflateEnd(&strm);
    return ret == Z_STREAM_END ? true : false;
}

ExtractProgress::~ExtractProgress()
{
    delete ui;
}
