/*
 * (c) 2014-2015 Sam Nazarko
 * email@samnazarko.co.uk
*/
#include "target.h"
#include <QString>

Target::Target(QString bootPath, QString bootFS, bool bootRW, QString rootPath)
{
    this->bootPath = bootPath;
    this->bootFS = bootFS;
    this->bootRW = bootRW;
    this->rootPath = rootPath;
}

void Target::setRoot(QString newRoot)
{
    this->rootPath = newRoot;
    rootChanged = true;
}
