#ifndef LANGSELECTION_H
#define LANGSELECTION_H

#include <QWidget>

namespace Ui {
class LangSelection;
}

class LangSelection : public QWidget
{
    Q_OBJECT

public:
    explicit LangSelection(QWidget *parent = 0);
    ~LangSelection();

private slots:
    void on_languagenextButton_clicked();

signals:
    void languageSelected(QString, QString);

private:
    Ui::LangSelection *ui;
};

#endif // LANGSELECTION_H
