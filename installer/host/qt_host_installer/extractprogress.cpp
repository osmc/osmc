#include "extractprogress.h"
#include "ui_extractprogress.h"
#include <QString>
#include <QByteArray>
#include "zlib.h"
#include "utils.h"
#include "assert.h"
#include "io.h"
#define CHUNKSIZE 8192
#define SET_BINARY_MODE(file)

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
        //writeImageToDisc(devicePath, deviceImage);

    }
    /* Peform pre-seeding operations and final configuration */
}

bool ExtractProgress::writeImageToDisc(QString devicePath, QString deviceImage)
{
    #ifdef Q_OS_MAC
    io::writeImageOSX(devicePath, deviceImage);
    #endif
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


    QFile sourceFile(deviceImage);
    bool sourceopen = sourceFile.open(QIODevice::ReadOnly);
    int sourceFileDescriptor = sourceFile.handle();
    FILE *source = fdopen(sourceFileDescriptor, "rb");

    QFile targetFile(QString(deviceImage).remove(".gz"));
    bool targetopen = targetFile.open(QIODevice::WriteOnly);
    int targetFileDescriptor = targetFile.handle();
    FILE *dest = fdopen(targetFileDescriptor, "wb");

    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    ret = inflateInit2(&strm,  47);
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
