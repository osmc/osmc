#ifndef EXTRACTPROGRESS_H
#define EXTRACTPROGRESS_H

#include <QWidget>
#include <QString>

namespace Ui {
class ExtractProgress;
}

class ExtractProgress : public QWidget
{
    Q_OBJECT
    
public:
    explicit ExtractProgress(QWidget *parent = 0, QString devicePath = NULL, QString deviceImage = NULL );
    ~ExtractProgress();
    
private:
    Ui::ExtractProgress *ui;
    bool doExtraction(QString deviceImage);
    bool writeImageToDisc(QString devicePath, QString deviceImage);
};

#endif // EXTRACTPROGRESS_H
