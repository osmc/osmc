#ifndef BOOTLOADERCONFIG_H
#define BOOTLOADERCONFIG_H
#include "target.h"
#include "network.h"
#include "utils.h";
#include "logger.h"

class BootloaderConfig
{
public:
    BootloaderConfig(Target *device, Network *network, Utils *utils, Logger *logger);
    void copyBootFiles();
    void configureMounts();
    void configureEnvironment();

private:
    Utils *utils;
    Target *device;
    Network *network;
    Logger *logger;
};

#endif // BOOTLOADERCONFIG_H
