#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMovie>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    
private:
    Ui::MainWindow *ui;
    void setLabelText(QString labelString);
    bool installFailed;
    bool installPartitioning;
    bool installSD;
    bool installUSB;
    bool installNFS;
    QMovie *movie;
};

#endif // MAINWINDOW_H
