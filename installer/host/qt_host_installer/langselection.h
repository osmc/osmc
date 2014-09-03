#ifndef LANGSELECTION_H
#define LANGSELECTION_H

#include <QWidget>
#include <QList>
#include "supporteddevice.h"

namespace Ui {
class LangSelection;
}

class LangSelection : public QWidget
{
    Q_OBJECT

public:
    explicit LangSelection(QWidget *parent = 0, QList<SupportedDevice *> devices = QList<SupportedDevice *>());
    ~LangSelection();

private slots:
    void on_languagenextButton_clicked();

signals:
    void languageSelected(QString, SupportedDevice);

private:
    Ui::LangSelection *ui;
    QList<SupportedDevice *> devicesList;
};

#endif // LANGSELECTION_H
