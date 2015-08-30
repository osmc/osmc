/*
 * (c) 2014-2015 Sam Nazarko
 * email@samnazarko.co.uk
*/
#ifndef BOOTLOADERCONFIG_H
#define BOOTLOADERCONFIG_H
#include "target.h"
#include "network.h"
#include "utils.h";
#include "logger.h"
#include "preseedparser.h"

class BootloaderConfig
{
public:
    BootloaderConfig(Target *device, Network *network, Utils *utils, Logger *logger, PreseedParser *preseed);
    void copyBootFiles();
    void configureMounts();
    void configureEnvironment();

private:
    Utils *utils;
    Target *device;
    Network *network;
    Logger *logger;
    PreseedParser *preseed;
};

#endif // BOOTLOADERCONFIG_H
