#include "bootloaderconfig.h"
#include "target.h"
#include <cstdlib>
#include "network.h"
#include "utils.h"
#include <QStringList>

BootloaderConfig::BootloaderConfig(Target *device, Network *network)
{
    this->device = device;
    this->network = network;
}

void BootloaderConfig::copyBootFiles()
{
    system("mv -ar /mnt/boot/* /mnt/root");
}

void BootloaderConfig::configureFstab()
{
    QFile fstabFile("/rfs/etc/fstab");
    QStringList fstabStringList;
    if (utils::getOSMCDev() == "rbp")
    {
        fstabStringList.append(device->getBoot() + "  /boot" + "    " + device->getBootFS() + "     defaults,noatime    0   0");
        if (! device->getRoot().contains(":/"))
            fstabStringList.append(device->getRoot() + "  /" + "    " + "ext4" + "      defaults,noatime    0   0" );
        else
            /* NFS install */
            fstabStringList.append("/dev/nfs   /      auto       defaults,noatime    0   0");
    }
    utils::writeToFile(fstabFile, fstabStringList, true);
}

void BootloaderConfig::configureCmdline()
{
    if (utils::getOSMCDev() == "rbp")
    {
        QFile cmdlineFile("/mnt/boot/cmdline.txt");
        QStringList cmdlineStringList;
        if (! device->getRoot().contains(":/"))
            cmdlineStringList << "root=" + this->device->getRoot() + " rootfstype=ext4 rootwait quiet";
        else
        {
            /* NFS install */
            cmdlineStringList << "root=/dev/nfs nfsroot=" + this->device->getRoot() + "ip=" + ((network->isDefined() == false) ? "dhcp" : network->getIP() + "::" + network->getGW() + ":" + network->getMask() + ":osmc:eth0:off") + " rootwait quiet";
        }
        utils::writeToFile(cmdlineFile, cmdlineStringList, false);
    }
}
