#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "langselection.h"
#include "ui_langselection.h"
#include "utils.h"
#include <QString>

QString language;
QString device;
LangSelection *ls;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    this->setFixedSize(this->size());
    ui->setupUi(this);
    ls = new LangSelection(this);
    connect(ls, SIGNAL(languageSelected(QString, QString)), this, SLOT(setLanguage(QString, QString)));
    ls->move(QPoint(10,110));
}

void MainWindow::setLanguage(QString language, QString device)
{
        utils::writeLog("The user has selected " + language + " as their language");
        utils::writeLog("The user has selected " + device + " as their device");
        language = language;
        device = device;
        QTranslator translator;
        translator.load(QString("osmc_da.qm"));
        qApp->installTranslator(&translator);
        ls->hide();
}

MainWindow::~MainWindow()
{
    delete ui;
}
