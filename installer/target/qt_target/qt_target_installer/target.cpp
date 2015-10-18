/*
 * (c) 2014-2015 Sam Nazarko
 * email@samnazarko.co.uk
*/
#include "target.h"
#include <QString>

Target::Target(QString bootPath, bool bootNeedsFormat, QString bootFS, bool bootRW, QString rootPath)
{
    this->bootPath = bootPath;
    this->bootNeedsFormat = bootNeedsFormat;
    this->bootFS = bootFS;
    this->bootRW = bootRW;
    this->rootPath = rootPath;
}

void Target::setRoot(QString newRoot)
{
    this->rootPath = newRoot;
    rootChanged = true;
}

void Target::setBootNeedsFormat(bool needsFormat)
{
    this->bootNeedsFormat = needsFormat;
}
