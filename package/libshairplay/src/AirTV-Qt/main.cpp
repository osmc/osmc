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

#include <QtGui>
#include <QtSingleApplication>

#include "mainapplication.h"
#include "videowidget.h"
#include "raopservice.h"

int main(int argc, char *argv[])
{
    QtSingleApplication a(argc, argv);
    if (a.isRunning()) {
        return 0;
    }
    a.setApplicationName("AirTV");

    if (!QSystemTrayIcon::isSystemTrayAvailable())  {
        QMessageBox::critical(0, QObject::tr("Systray"),
                              QObject::tr("I couldn't detect any system tray "
                                          "on this system."));
        return 1;
    }
    QApplication::setQuitOnLastWindowClosed(false);

    MainApplication m;
    QObject::connect(&m, SIGNAL(quitRequested()), &a, SLOT(quit()));
    QObject::connect(&a, SIGNAL(aboutToQuit()), &m, SLOT(aboutToQuit()));

    if(m.start()) {
        return a.exec();
    } else {
        return EXIT_FAILURE;
    }
}
