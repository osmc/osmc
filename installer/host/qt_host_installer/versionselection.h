#ifndef VERSIONSELECTION_H
#define VERSIONSELECTION_H

#include <QWidget>
#include "supporteddevice.h"

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
    void versionSelected(QString version);

private slots:
    void on_versionnextButton_clicked();

    void on_useLocalBuildCheckbox_stateChanged(int arg1);

private:
    Ui::VersionSelection *ui;
    QString version;
};

#endif // VERSIONSELECTION_H
