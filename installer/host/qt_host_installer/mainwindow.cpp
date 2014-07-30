#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    this->setStyleSheet("background-color: #17394A");
    this->setFixedSize(this->size());
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}
