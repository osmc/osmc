#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFile>
#include <QDir>
#include "utils.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setFixedSize(this->size());
    /* Perform installation to target */
}

void MainWindow::setLabelText(QString labelString)
{
    ui->statusLabel->setText(labelString);
}

MainWindow::~MainWindow()
{
    delete ui;
}
