#include "bootloaderconfig.h"
#include "target.h"
#include <cstdlib>
#include "network.h"
#include <QStringList>

BootloaderConfig::BootloaderConfig(Target *device, Network *network, Utils *utils, Logger *logger)
{
    this->device = device;
    this->network = network;
    this->utils = utils;
    this->logger = logger;
}

void BootloaderConfig::copyBootFiles()
{
    system("mv /mnt/boot/preseed.cfg /tmp/preseed.cfg");
    system("rm -rf /mnt/boot/*"); /* Trash existing files */
    system("mv /tmp/preseed.cfg /mnt/boot/preseed.cfg");
    system("mv /mnt/root/boot/* /mnt/boot");
}

void BootloaderConfig::configureMounts()
{
    QFile fstabFile("/mnt/root/etc/fstab");
    QStringList fstabStringList;
    if (utils->getOSMCDev() == "rbp")
    {
        fstabStringList.append(device->getBoot() + "  /boot" + "    " + device->getBootFS() + "     defaults,noatime    0   0\n");
        if (! device->getRoot().contains(":/"))
            fstabStringList.append(device->getRoot() + "  /" + "    " + "ext4" + "      defaults,noatime    0   0\n" );
        else
            /* NFS install */
            fstabStringList.append("/dev/nfs   /      auto       defaults,noatime    0   0\n");
    }
    utils->writeToFile(fstabFile, fstabStringList, true);
}

void BootloaderConfig::configureEnvironment()
{
    if (utils->getOSMCDev() == "rbp")
    {
        QFile cmdlineFile("/mnt/boot/cmdline.txt");
        QStringList cmdlineStringList;
        if (! device->getRoot().contains(":/"))
            cmdlineStringList << "root=" + this->device->getRoot() + " rootfstype=ext4 rootwait quiet";
        else
        {
            /* NFS install */
            cmdlineStringList << "root=/dev/nfs nfsroot=" + this->device->getRoot() + " ip=" + ((network->isDefined() == false) ? "dhcp" : network->getIP() + "::" + network->getGW() + ":" + network->getMask() + ":osmc:eth0:off") + " rootwait quiet";
        }
	/* Application Store identifier */
	cmdlineStringList << " osmcdev=rbp";
        utils->writeToFile(cmdlineFile, cmdlineStringList, false);
        QFile configFile("/mnt/boot/config.txt");
        QStringList configStringList;
        configStringList << "arm_freq=850\n" << "core_freq=375\n" << "gpu_mem_256=112\n" << "gpu_mem_512=144\n" << "hdmi_ignore_cec_init=1\n" << "disable_overscan=1\n" << "start_file=start_x.elf\n" << "fixup_file=fixup_x.dat";
        utils->writeToFile(configFile, configStringList, false);
    }
}
