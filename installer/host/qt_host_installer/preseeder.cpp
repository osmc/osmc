/*
 * (c) 2014-2015 Sam Nazarko
 * email@samnazarko.co.uk
*/
#include "preseeder.h"
#include "utils.h"
#include <QStringList>
#include "networksettings.h"
#include "mainwindow.h"
#include <QString>

Preseeder::Preseeder()
{
    preseedStringList = QStringList();
}

void Preseeder::setLanguageString(QString locale)
{
    writeOption("globe", "locale", PRESEED_STRING, locale);
}

void Preseeder::setNetworkSettings(NetworkSettings *ns)
{
    writeOption("network", "interface", PRESEED_STRING, ns->hasWireless() ? "wlan" : "eth");
    writeOption("network", "auto", PRESEED_BOOL, ns->hasDHCP() ? "true" : "false");
    if ( ! ns->hasDHCP())
    {
        writeOption("network", "ip", PRESEED_STRING, ns->getIP());
        writeOption("network", "mask", PRESEED_STRING, ns->getMask());
        writeOption("network", "dns1", PRESEED_STRING, ns->getDNS1());
        writeOption("network", "dns2", PRESEED_STRING, ns->getDNS2());
        writeOption("network", "gw", PRESEED_STRING, ns->getGW());
    }
    if (ns->hasWireless())
    {
        writeOption("network", "ssid", PRESEED_STRING, ns->getWirelessSSID());
        writeOption("network", "wlan_keytype", PRESEED_STRING, QString::number(ns->getWirelessKeyType()));
        writeOption("network", "wlan_key", PRESEED_STRING, ns->getWirelessKeyValue());
    }
}

void Preseeder::setTargetSettings(MainWindow *mw)
{
    if (mw->getInstallType() == utils::INSTALL_SD)
        writeOption("target", "storage", PRESEED_STRING, "sd");
    if (mw->getInstallType() == utils::INSTALL_USB)
        writeOption("target", "storage", PRESEED_STRING, "usb");
    if (mw->getInstallType() == utils::INSTALL_EMMC)
        writeOption("target", "storage", PRESEED_STRING, "emmc");
    if (mw->getInstallType() == utils::INSTALL_NFS)
    {
        writeOption("target", "storage", PRESEED_STRING, "nfs");
        writeOption("target", "storagePath", PRESEED_STRING, mw->getNFSPath());
    }
    if (mw->getInstallType() == utils::INSTALL_NOPRESEED)
        writeOption("target", "storage", PRESEED_STRING, "nops");
    if (mw->getInstallType() == utils::INSTALL_PARTITIONER)
        writeOption("target", "storage", PRESEED_STRING, "partition");
}

void Preseeder::writeOption(QString preseedSection, QString preseedOptionKey, int preseedOptionType, QString preseedOptionValue)
{
    QString toWrite;
    toWrite = "d-i " + preseedSection + "/" + preseedOptionKey;
    switch (preseedOptionType)
    {
    case PRESEED_STRING:
        toWrite += " string ";
        break;
    case PRESEED_BOOL:
        toWrite += " boolean ";
        break;
    }
    toWrite += preseedOptionValue;
    utils::writeLog("Adding preseed string" + toWrite);
    preseedStringList << toWrite;
}

