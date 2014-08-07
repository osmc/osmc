# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

echo -e "Building package rpi-bootloader"
BOOT="files/boot"
FWFILES="LICENCE.broadcom
start_x.elf
fixup_x.dat
bootcode.bin"
make clean

mkdir -p "${BOOT}"

for file in $FWFILES
do
	wget --no-check-certificate https://raw.githubusercontent.com/raspberrypi/firmware/master/boot/"${file}" -O "${BOOT}"/"${file}"
	if [ $? != 0 ]; then echo "Download failed" && exit 1; fi
done

dpkg -b files/ osmc-rpi-bootloader.deb
