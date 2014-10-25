#ifndef BOOTLOADERCONFIG_H
#define BOOTLOADERCONFIG_H
#include "target.h"
#include "network.h"

class BootloaderConfig
{
public:
    BootloaderConfig(Target *device, Network *network);
    Target *device;
    Network *network;
    void copyBootFiles();
    void configureFstab();
    void configureCmdline();
};

#endif // BOOTLOADERCONFIG_H
