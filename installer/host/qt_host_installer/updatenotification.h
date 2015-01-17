/*
 * (c) 2014-2015 Sam Nazarko
 * email@samnazarko.co.uk
*/
#ifndef UPDATENOTIFICATION_H
#define UPDATENOTIFICATION_H

#include <QWidget>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>

namespace Ui {
class UpdateNotification;
}

class UpdateNotification : public QWidget
{
    Q_OBJECT

public:
    explicit UpdateNotification(QWidget *parent = 0);
    ~UpdateNotification();
    void isUpdateAvailable(QString &baseURL);

private slots:
    void on_dismissButton_clicked() { emit ignoreUpdate(); }
    void replyFinished(QNetworkReply* reply);

signals:
    void ignoreUpdate();
    void hasUpdate();

private:
    Ui::UpdateNotification *ui;
    QNetworkAccessManager *accessManager;
};

#endif // UPDATENOTIFICATION_H
