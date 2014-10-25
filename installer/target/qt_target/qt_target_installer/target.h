#ifndef TARGET_H
#define TARGET_H
#include <QString>

class Target
{
public:
    Target(QString bootPath, QString bootFS, bool bootRW, QString rootPath);
    QString getBoot() { return bootPath; }
    QString getBootFS() { return bootFS; }
    bool isBootRW() { return bootRW; }
    bool hasRootChanged() { return rootChanged; }
    QString getRoot() { return rootPath; }
    void setRoot(QString newRoot); /* Allows overriding root */

private:
    QString bootPath;
    QString bootFS;
    QString rootPath;
    bool bootRW;
    bool rootChanged = false; /* So we know if we need to mklabel */
};

#endif // TARGET_H
