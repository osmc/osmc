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
    
private:
    Ui::MainWindow *ui;
    void setLabelText(QString labelString);
    bool installFailed = false;
    bool installPartitioning = false;
    bool installSD = false;
    bool installUSB = false;
    bool installNFS = false;
};

#endif // MAINWINDOW_H
