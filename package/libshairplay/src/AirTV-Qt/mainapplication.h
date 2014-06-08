/**
 *  Copyright (C) 2011-2012  Juho Vähä-Herttua
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 */

#ifndef MAINAPPLICATION_H
#define MAINAPPLICATION_H

#include <QObject>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>

#include "raopservice.h"
#include "dnssdservice.h"
#include "audiocallbacks.h"

class MainApplication : public QObject
{
    Q_OBJECT
public:
    explicit MainApplication(QObject *parent = 0);
    ~MainApplication();

    bool start();
    void stop();

private:
    RaopService *raopService;
    DnssdService *dnssdService;
    AudioCallbacks m_callbacks;

    QSystemTrayIcon *trayIcon;
    QMenu *trayIconMenu;
    QAction *quitAction;

signals:
    void quitRequested();

public slots:
    void aboutToQuit();
};

#endif // MAINAPPLICATION_H
