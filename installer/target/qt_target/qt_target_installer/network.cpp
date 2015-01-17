/*
 * (c) 2014-2015 Sam Nazarko
 * email@samnazarko.co.uk
*/
#include "network.h"
#include <QStringList>
#include <QFile>
#include <QTextStream>
#include <QProcess>
#include <cstdlib>

Network::Network()
{
}

void Network::bringUp()
{
    QStringList interfacesStringList = QStringList();
    interfacesStringList.append(QString("auto eth0"));
    /* If we aren't using DHCP */
    if (this->isDefined())
    {
        interfacesStringList.append("iface eth0 inet static");
        interfacesStringList.append("\t address " + this->getIP());
        interfacesStringList.append("\t netmask " + this->getMask());
        interfacesStringList.append("\t gateway " + this->getGW());
        QFile nameserversFile("/etc/resolv.conf");
        nameserversFile.open(QIODevice::WriteOnly, QIODevice::Text);
        QTextStream nameserversTextStream(&nameserversFile);
        nameserversTextStream << "nameserver " + this->getDNS1();
        nameserversTextStream << "nameserver " + this->getDNS2();
        nameserversFile.close();
    }
    else
    {
        interfacesStringList.append(QString("iface eth0 inet dhcp"));
    }
    /* Write to /etc/network/interfaces */
    QFile interfacesFile("/etc/network/interfaces");
    interfacesFile.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream interfacesStream(&interfacesFile);
    for (int i = 0; i < interfacesStringList.count(); i++)
    {
        interfacesStream << interfacesStringList.at(i) + "\n";
    }
    interfacesFile.close();
    /* And finally, bring up */
    system("/sbin/ifup eth0");
}
