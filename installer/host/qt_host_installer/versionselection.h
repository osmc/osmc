#ifndef VERSIONSELECTION_H
#define VERSIONSELECTION_H

#include <QWidget>
#include "supporteddevice.h"
#include <QtNetwork/QNetworkAccessManager>
#include <QMap>
#include <QUrl>
#include <QString>

namespace Ui {
class VersionSelection;
}

class VersionSelection : public QWidget
{
    Q_OBJECT

public:
    explicit VersionSelection(QWidget *parent = 0, QString deviceShortName = NULL);
    ~VersionSelection();

signals:
    void versionSelected(bool isOnline, QUrl image);

private slots:
    void on_versionnextButton_clicked();
    void replyFinished(QNetworkReply* reply);
    void on_useLocalBuildCheckbox_stateChanged(int arg1);

private:
    Ui::VersionSelection *ui;
    QString version;
    QString buildName;
    QNetworkAccessManager *accessManager;
    void enumerateBuilds(QByteArray buildline);
    QMap<QString,QString> buildMap;
};

#endif // VERSIONSELECTION_H
