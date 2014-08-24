#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    void setLanguage(QString, QString);
    void dismissUpdate();
    
private:
    Ui::MainWindow *ui;
    void translate(QString locale);
};

#endif // MAINWINDOW_H
