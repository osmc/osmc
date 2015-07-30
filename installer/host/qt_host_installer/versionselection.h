/*
 * (c) 2014-2015 Sam Nazarko
 * email@samnazarko.co.uk
*/
#ifndef VERSIONSELECTION_H
#define VERSIONSELECTION_H

#include <QWidget>
#include "supporteddevice.h"
#include <QtNetwork/QNetworkAccessManager>
#include <QMap>
#include <QUrl>
#include <QString>
#include "ui_versionselection.h"

namespace Ui {
class VersionSelection;
}

class VersionSelection : public QWidget
{
    Q_OBJECT

    virtual void showEvent(QShowEvent *event)
    {
        ui->versionnextButton->setEnabled(true);
    }

public:
    explicit VersionSelection(QWidget *parent = 0, QString deviceShortName = NULL, QString mirrorURL = NULL);
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
    bool enumerateBuilds(QByteArray buildline);
    QMap<QString,QString> buildMap;
};

#endif // VERSIONSELECTION_H
