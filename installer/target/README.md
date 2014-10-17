#Target installer
This installer is run on the actual device that OSMC is being loaded on. It offers the following important capabilities:

* Preseeding
* Partitioning

The installer will extract the filesystem, configure the bootloader, configure any preseeding and then hand off to the target root filesystem. This allows for dynamic preseeding options that are not usually available in a standard disk image. 

It is based on upstream [Buildroot] (http://buildroot.uclibc.org). The root filesystem embeds Qt 4 (leveraging QWS) which is embedded in to an initramfs. This allows us graphical applications without the heavy dependency of X.

Simply run make for a list of supported targets.  

##Running locally

The installer can be tested on a Linux system with Qt, as it uses QWS preprocessor directives to determine whether or not it is actually running on a device.

For testing purposes, our installer will emulate a Raspberry Pi performing an SD card install by default. Check the comments and log output for well documented information on where OSMC's target installer has 'faked' an install step. 
