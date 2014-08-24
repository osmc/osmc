#ifndef UPDATENOTIFICATION_H
#define UPDATENOTIFICATION_H

#include <QWidget>

namespace Ui {
class UpdateNotification;
}

class UpdateNotification : public QWidget
{
    Q_OBJECT

public:
    explicit UpdateNotification(QWidget *parent = 0);
    ~UpdateNotification();
    static bool isUpdateAvailable();

private slots:
    void on_dismissButton_clicked() { emit ignoreUpdate(); }

signals:
    void ignoreUpdate();

private:
    Ui::UpdateNotification *ui;
};

#endif // UPDATENOTIFICATION_H
