# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

echo -e "Building package rbp-bootloader-osmc"

BOOT="files/boot"
FWFILES=( "LICENCE.broadcom" "start_x.elf" "fixup_x.dat" "bootcode.bin" )
REV="cf20cb2554fef392da5d355029bcb06b841f2b2e"

make clean

mkdir -p "${BOOT}"

for file in ${FWFILES[@]}
do
    pull_bin "https://raw.githubusercontent.com/raspberrypi/firmware/${REV}/boot/${file}" "${BOOT}/${file}"
done

dpkg_build files/ rbp-bootloader-osmc.deb
