# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

echo -e "Building package rbp-bootloader-osmc"

BOOT="files/boot"
FWFILES=( "LICENCE.broadcom" "start_x.elf" "fixup_x.dat" "bootcode.bin" )
REV="87edb826b91ce08ea3e383936cc4791eb7491f35"

make clean

mkdir -p "${BOOT}"

for file in ${FWFILES[@]}
do
    pull_bin "https://raw.githubusercontent.com/raspberrypi/firmware/${REV}/boot/${file}" "${BOOT}/${file}"
done

dpkg_build files/ rbp-bootloader-osmc.deb
