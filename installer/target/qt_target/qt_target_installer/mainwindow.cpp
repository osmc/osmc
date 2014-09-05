#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "utils.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setFixedSize(this->size());
    movie = new QMovie(":/assets/resources/BOOTLOOP.gif");
    ui->animationLabel->setMovie(movie);
    movie->start();
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
