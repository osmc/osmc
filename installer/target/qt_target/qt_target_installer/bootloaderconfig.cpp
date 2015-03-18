/*
 * (c) 2014-2015 Sam Nazarko
 * email@samnazarko.co.uk
*/
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
    if (utils->getOSMCDev() == "rbp1" || utils->getOSMCDev() == "rbp2" || utils->getOSMCDev() == "vero1")
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
    if (utils->getOSMCDev() == "rbp1" || utils->getOSMCDev() == "rbp2")
    {
        QFile cmdlineFile("/mnt/boot/cmdline.txt");
        QStringList cmdlineStringList;
        if (! device->getRoot().contains(":/"))
            cmdlineStringList << "root=" + this->device->getRoot() + " rootfstype=ext4 rootwait quiet ";
        else
        {
            /* NFS install */
            cmdlineStringList << "root=/dev/nfs nfsroot=" + this->device->getRoot() + " ip=" + ((network->isDefined() == false) ? "dhcp" : network->getIP() + "::" + network->getGW() + ":" + network->getMask() + ":osmc:eth0:off") + ":" + network->getDNS1() + ":" + network->getDNS2() + " rootwait quiet ";
        }
        QFile configFile("/mnt/boot/config.txt");
        QStringList configStringList;
        if (utils->getOSMCDev() == "rbp1")
        {
            configStringList << "arm_freq=850\n" << "core_freq=375\n" << "gpu_mem_256=112\n" << "gpu_mem_512=144\n" << "hdmi_ignore_cec_init=1\n" << "disable_overscan=1\n" << "start_x=1\n";
            cmdlineStringList << "osmcdev=rbp1";
        }
        if (utils->getOSMCDev() == "rbp2")
        {
            configStringList << "gpu_mem_1024=256\n" << "hdmi_ignore_cec_init=1\n" << "disable_overscan=1\n" << "start_x=1\n";
            cmdlineStringList << "osmcdev=rbp2";
        }
        utils->writeToFile(configFile, configStringList, false);
        utils->writeToFile(cmdlineFile, cmdlineStringList, false);
        configFile.close();
        cmdlineFile.close();
    }
    if (utils->getOSMCDev() == "vero1")
    {
        QFile uEnvFile("/mnt/boot/uEnv.txt");
        QStringList uEnvStringList;
        if (! device->getRoot().contains(":/"))
            uEnvStringList << "mmcargs=setenv bootargs 'root=" + this->device->getRoot() + " rootfstype=ext4 rootwait quiet video=mxcfb0:dev=hdmi,1920x1080M@60,if=RGB24,bpp=32 dmfc=3 consoleblank=0 ";
        else
        {
            /* NFS install */
            uEnvStringList << "root=/dev/nfs nfsroot=" + this->device->getRoot() + " ip=" + ((network->isDefined() == false) ? "dhcp" : network->getIP() + "::" + network->getGW() + ":" + network->getMask() + ":osmc:eth0:off") + ":" + network->getDNS1() + ":" + network->getDNS2() + " rootwait quiet video=mxcfb0:dev=hdmi,1920x1080M@60,if=RGB24,bpp=32 dmfc=3 consoleblank=0 ";
        }
        uEnvStringList << "osmcdev=vero1'";
        utils->writeToFile(uEnvFile, uEnvStringList, false);
        uEnvFile.close();
    }
}
