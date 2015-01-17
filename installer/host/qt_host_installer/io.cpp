/*
 * (c) 2014-2015 Sam Nazarko
 * email@samnazarko.co.uk
*/
#include "utils.h"
#include <QByteArray>
#include <QFile>

namespace io
{
    int getDecompressedSize(QString gzFilename)
    {
        /*
            Read the last four bytes of the given file and interpret them
            to be the size in bytes of the uncompressed gzip file.

            This method does not test if the given filename denotes
            an actual gzip file.

            This method does not care about endianess.

            Return the size in bytes.
            Returns -1 if the file could not be opened.
        */

        /* size of the uncompressed file can be found
         * in the last four bytes. It doesn't seem to be too exact
         * http://www.abeel.be/content/determine-uncompressed-size-gzip-file
         */

        QFile sourceFile(gzFilename);
        bool sourceopen = sourceFile.open(QIODevice::ReadOnly);
        if (!sourceopen)
        {
            utils::writeLog("io:getDecompressedSize: Could not open file "
                            + gzFilename
                            + " to determine decompressed size");
            return -1;
        }

        QByteArray buffer = sourceFile.readAll();
        int b4 = buffer.at(buffer.size()-4);
        int b3 = buffer.at(buffer.size()-3);
        int b2 = buffer.at(buffer.size()-2);
        int b1 = buffer.at(buffer.size()-1);
        int val = (b1 << 24) | (b2 << 16) + (b3 << 8) + b4;
        sourceFile.close();

        return val;
    }

    qint64 getFileSize(QString filename)
    {
        QFile file(filename);
        if(file.exists()) {
            return file.size();
        }
        return 0;
    }

}
