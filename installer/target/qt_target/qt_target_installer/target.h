/*
 * (c) 2014-2015 Sam Nazarko
 * email@samnazarko.co.uk
*/
#ifndef TARGET_H
#define TARGET_H
#include <QString>

class Target
{
public:
    Target(QString bootPath, bool bootNeedsFormat, QString bootFS, bool bootRW, QString rootPath, bool usesGPT);
    QString getBoot() { return bootPath; }
    QString getBootFS() { return bootFS; }
    bool isBootRW() { return bootRW; }
    bool hasRootChanged() { return rootChanged; }
    bool hasBootChanged() { return bootChanged; }
    void setBootNeedsFormat(bool needsFormat);
    bool doesBootNeedsFormat() { return bootNeedsFormat; }
    QString getRoot() { return rootPath; }
    void setRoot(QString newRoot); /* Allows overriding root */
    void setBoot(QString newBoot); /* Allows overriding boot */
    bool deviceUsesGPT() { return usesGPT; }

private:
    QString bootPath;
    bool bootNeedsFormat;
    QString bootFS;
    QString rootPath;
    bool bootRW;
    bool rootChanged = false; /* So we know if we need to mklabel */
    bool bootChanged = false; /* So we know to remount boot */
    bool usesGPT;
};

#endif // TARGET_H
