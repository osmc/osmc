#include <QString>
#include <QString>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QTranslator>
#include <QApplication>

namespace utils
{
    void writeLog(QString strLog);
    void loadTranslation(QString locale, QApplication *application);
    void displayError(QString title, QString message);
}
