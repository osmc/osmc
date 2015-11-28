/*
 * (c) 2014-2015 Sam Nazarko
 * email@samnazarko.co.uk
*/
#include "bootloaderconfig.h"
#include "target.h"
#include <cstdlib>
#include "network.h"
#include <QStringList>
#include <QTextStream>

BootloaderConfig::BootloaderConfig(Target *device, Network *network, Utils *utils, Logger *logger, PreseedParser *preseed)
{
    this->device = device;
    this->network = network;
    this->utils = utils;
    this->logger = logger;
    this->preseed = preseed;
}

void BootloaderConfig::copyBootFiles()
{
    if (utils->getOSMCDev() == "atv")
    {
        system("mv /mnt/boot/BootLogo.png /tmp/BootLogo.png");
        system("mv /mnt/boot/boot.efi /tmp/boot.efi");
        system("mv /mnt/boot/System /tmp/System");
    }
    system("mv /mnt/boot/preseed.cfg /tmp/preseed.cfg");
    system("rm -rf /mnt/boot/*"); /* Trash existing files */
    system("mv /tmp/preseed.cfg /mnt/boot/preseed.cfg");
    system("mv /mnt/root/boot/* /mnt/boot");
    if (utils->getOSMCDev() == "atv")
    {
        system("mv /tmp/BootLogo.png /mnt/boot/BootLogo.png");
        system("mv /tmp/boot.efi /mnt/boot/boot.efi");
        system("mv /tmp/System /mnt/boot");
    }
}

void BootloaderConfig::configureMounts()
{
    QFile fstabFile("/mnt/root/etc/fstab");
    QStringList fstabStringList;
    if (utils->getOSMCDev() == "rbp1" || utils->getOSMCDev() == "rbp2" || utils->getOSMCDev() == "vero1" || utils->getOSMCDev() == "atv")
    {
        QString bootFS = device->getBootFS();
        if (bootFS == "fat32") { bootFS = "vfat"; }
        fstabStringList.append(device->getBoot() + "  /boot" + "    " + bootFS + "     defaults,noatime    0   0\n");
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
            cmdlineStringList << "root=/dev/nfs nfsroot=" + this->device->getRoot();
            if (network->isDefined() == false)
                cmdlineStringList << " ip=dhcp";
            else
                cmdlineStringList << " ip=" + network->getIP() + "::" + network->getGW() + ":" + network->getMask() + ":osmc:eth0:off:" + network->getDNS1() + ":" + network->getDNS2();
            cmdlineStringList << " rootwait quiet ";
        }
        QFile configFile("/mnt/boot/config.txt");
        QStringList configStringList;
        if (utils->getOSMCDev() == "rbp1")
        {
            QFile cpuinfo("/proc/cpuinfo");
            int rev = 0x0000;
            if (cpuinfo.open(QIODevice::ReadOnly))
            {
                QTextStream in(&cpuinfo);
                while (!in.atEnd())
                {
                    QString line = in.readLine();
                    if (line.contains("Revision"))
                    {
                        line.replace(" ", "");
                        QStringList lines = line.split(":");
                        rev = lines[1].toUInt(NULL, 16);
                        break;
                    }

                }
                cpuinfo.close();
            }
            if ((rev >> 23) & 1)
            {
                if (rev >> 4 & 0x7F != 9)
                       /* Not a Pi Zero */
                       configStringList << "arm_freq=850\n" << "core_freq=375\n";
            }
            else
            {
                /* Not a Pi Zero */
                configStringList << "arm_freq=850\n" << "core_freq=375\n";
            }
            configStringList << "gpu_mem_256=112\n" << "gpu_mem_512=144\n" << "hdmi_ignore_cec_init=1\n" << "disable_overscan=1\n" << "start_x=1\n" << "disable_splash=1\n";
            cmdlineStringList << "osmcdev=rbp1";
        }
        if (utils->getOSMCDev() == "rbp2")
        {
            configStringList << "gpu_mem_1024=256\n" << "hdmi_ignore_cec_init=1\n" << "disable_overscan=1\n" << "start_x=1\n" << "disable_splash=1\n";
            cmdlineStringList << "osmcdev=rbp2";
        }
        if (preseed->getBoolValue("vendor/dtoverlay"))
            configStringList << "dtoverlay=" << preseed->getStringValue("vendor/dtoverlayparam") << "\n";
        else
            configStringList << "dtoverlay=lirc-rpi:gpio_out_pin=17,gpio_in_pin=18\n";
        utils->writeToFile(configFile, configStringList, false);
        utils->writeToFile(cmdlineFile, cmdlineStringList, false);
        configFile.close();
        cmdlineFile.close();
    }
    if (utils->getOSMCDev() == "vero1") /* We only use 1x identifier for WiFi chip, so make it 'vero' later */
    {
        QFile uEnvFile("/mnt/boot/uEnv.txt");
        QStringList uEnvStringList;
        if (! device->getRoot().contains(":/"))
            uEnvStringList << "mmcargs=setenv bootargs console=tty1 root=" + this->device->getRoot() + " rootfstype=ext4 rootwait quiet video=mxcfb0:dev=hdmi,1920x1080M@60,if=RGB24,bpp=32 dmfc=3 consoleblank=0 loglevel=2 ";
        else
        {
            /* NFS install */
            uEnvStringList << "mmcargs=setenv bootargs console=tty1 root=/dev/nfs nfsroot=" + this->device->getRoot();
            if (network->isDefined() == false)
                uEnvStringList << " ip=dhcp";
            else
                uEnvStringList << " ip=" + network->getIP() + "::" + network->getGW() + ":" + network->getMask() + ":osmc:eth0:off:" + network->getDNS1() + ":" + network->getDNS2();
            uEnvStringList << " rootwait quiet video=mxcfb0:dev=hdmi,1920x1080M@60,if=RGB24,bpp=32 dmfc=3 consoleblank=0 loglevel=2 ";
        }
        uEnvStringList << "osmcdev=vero";
        utils->writeToFile(uEnvFile, uEnvStringList, false);
        uEnvFile.close();
    }
   if (utils->getOSMCDev() == "atv")
   {
       QFile bootListFile("/mnt/boot/com.apple.Boot.plist");
       QStringList bootStringList;
       bootListFile.close();
       bootStringList << "<?xml version=\"1.0\" encoding=\"UTF-8\">" << "\n";
       bootStringList << "<!DOCTYPE plist PUBLIC \"-//Apple Computer//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">" << "\n";
       bootStringList << "<plist version=\"1.0\">" << "\n";
       bootStringList << "<dict>" << "\n";
       bootStringList << "      <key>Background Color</key>" << "\n";
       bootStringList << "      <integer>0</integer>" << "\n";
       bootStringList << "      <key>Boot Logo</key>" << "\n";
       bootStringList << "      <string>BootLogo.png</string>" << "\n";
       bootStringList << "      <key>Kernel Flags</key>" << "\n";
       bootStringList << "      <string>console=tty1 root=" + this->device->getRoot() + " rootfstype=ext4 rootwait quiet video=vesafb intel_idle.max_cstate=1 processor.max_cstate=2 nohpet vga16fb.modeset=0 osmcdev=atv" << "</string>" << "\n";
       bootStringList << "      <key>Kernel</key>" << "\n";
       bootStringList << "      <string>mach_kernel</string>" << "\n";
       bootStringList << "</dict>" << "\n";
       bootStringList << "</plist>" << "\n";
       utils->writeToFile(bootListFile, bootStringList, false);
   }
}
