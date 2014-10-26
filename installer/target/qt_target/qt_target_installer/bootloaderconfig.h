#ifndef BOOTLOADERCONFIG_H
#define BOOTLOADERCONFIG_H
#include "target.h"
#include "network.h"
#include "utils.h";

class BootloaderConfig
{
public:
    BootloaderConfig(Target *device, Network *network, Utils *utils);
    Target *device;
    Network *network;
    void copyBootFiles();
    void configureFstab();
    void configureCmdline();

private:
    Utils *utils;
};

#endif // BOOTLOADERCONFIG_H
