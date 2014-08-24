#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "langselection.h"
#include "ui_langselection.h"
#include "updatenotification.h"
#include "ui_updatenotification.h"
#include "utils.h"
#include <QString>
#include <QTranslator>
#define WIDGET_START QPoint(10,110)

QString language;
QString device;
UpdateNotification *updater;
LangSelection *ls;

QTranslator translator;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    this->setFixedSize(this->size());
    ui->setupUi(this);
    /* Attempt auto translation */
    QString autolocale = QLocale::system().name();
    utils::writeLog("Detected locale as " + autolocale);
    translate(autolocale);
    ls = new LangSelection(this);
    connect(ls, SIGNAL(languageSelected(QString, QString)), this, SLOT(setLanguage(QString, QString)));
    ls->move(WIDGET_START);
    /* Check if an update exists */
    if (UpdateNotification::isUpdateAvailable())
    {
        updater = new UpdateNotification(this);
        connect(updater, SIGNAL(ignoreUpdate()), this, SLOT(dismissUpdate()));
        updater->move(WIDGET_START);
        ls->hide();
    }
}

void MainWindow::dismissUpdate()
{
    updater->hide();
    ls->show();
}

void MainWindow::setLanguage(QString language, QString device)
{
        utils::writeLog("The user has selected " + language + " as their language");
        utils::writeLog("The user has selected " + device + " as their device");
        language = language;
        device = device;
        if (language != tr("English"))
        {
            translate(language);
        }
        else
        {
            /* Remove because we may have already done the deed */
            qApp->removeTranslator(&translator);
        }
        ls->hide();
}

void MainWindow::translate(QString locale)
{
    utils::writeLog("Attempting to load translation for locale " + locale);
    if (translator.load(qApp->applicationDirPath() + "/osmc_" + locale + ".qm"))
    {
        utils::writeLog("Translation loaded successfully");
        qApp->installTranslator(&translator);
        ui->retranslateUi(this);
    }
    else
        utils::writeLog("Could not load translation!");
}

MainWindow::~MainWindow()
{
    delete ui;
}
