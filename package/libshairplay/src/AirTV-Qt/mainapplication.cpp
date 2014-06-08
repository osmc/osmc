#include "mainapplication.h"

#include <QDebug>
#include <QCoreApplication>

MainApplication::MainApplication(QObject *parent) :
    QObject(parent)
{
    raopService = new RaopService(0);
    dnssdService = new DnssdService(0);
    trayIconMenu = new QMenu(0);

    quitAction = new QAction(tr("&Quit"), trayIconMenu);
    connect(quitAction, SIGNAL(triggered()), this, SIGNAL(quitRequested()));
    trayIconMenu->addAction(quitAction);

    // Construct the actual system tray icon
    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setContextMenu(trayIconMenu);
    trayIcon->setIcon(QIcon(":icons/airtv.svg"));
}

MainApplication::~MainApplication()
{
    trayIcon->setContextMenu(0);
    delete trayIconMenu;
    delete raopService;
}

bool MainApplication::start()
{
    // Initialize the service
    bool initSuccess = false;
    initSuccess = raopService->init(10, &m_callbacks);
    if(!initSuccess) {
        qDebug() << "Error initializing raop service";
        return false;
    }
    initSuccess &= dnssdService->init();
    if(!initSuccess) {
        qDebug() << "Error initializing dnssd service";
        return false;
    }

    char chwaddr[] = { 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB };
    QByteArray hwaddr(chwaddr, sizeof(chwaddr));

    raopService->start(5000, hwaddr);
    dnssdService->registerRaop("Shairplay", 5000, hwaddr);
    trayIcon->show();
    return true;
}

void MainApplication::stop()
{
    dnssdService->unregisterRaop();
    raopService->stop();
    trayIcon->hide();
}

void MainApplication::aboutToQuit()
{
    this->stop();
}
