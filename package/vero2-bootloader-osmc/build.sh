# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

echo -e "Building package vero2-bootloader-osmc"

BOOT="files/boot"
REV="4484dae9ca7afa9a6adf7f4974152212cc7a61cf"

make clean

mkdir -p "${BOOT}"

pull_bin "https://raw.githubusercontent.com/samnazarko/vero2-uboot/${REV}/bootloader.img" "${BOOT}/bootloader.img"

dpkg_build files/ vero2-bootloader-osmc.deb
