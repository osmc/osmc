# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

echo -e "Building package vero2-earlysplash-osmc"
dpkg_build files/ 

if [ $? != 0 ]; then echo -e "Error downloading" && exit 1; fi
# Build in native environment
build_in_env "${1}" $(pwd) "vero2-earlysplash-osmc"
build_return=$?
if [ $build_return == 99 ]
then
	echo -e "Building package vero2-earlysplash-osmc"
	out=$(pwd)/files
	make clean
	mkdir -p ${out}/boot
	pushd src/vero-bootloader*
	make mx6_vero_config
	$BUILD
	cp -ar u-boot.img ${out}/boot/
	cp -ar SPL ${out}/boot/
	if [ $? != 0 ]; then echo "Error occured during build" && exit 1; fi
	popd
	dpkg_build files/ vero1-bootloader-osmc.deb
	build_return=$?
fi
teardown_env "${1}"
exit $build_return

BOOT="files/boot"
FWFILES=( "LICENCE.broadcom" "start_x.elf" "fixup_x.dat" "bootcode.bin" )
REV="c6ed2bf9d507be18775d0a1764f6f79656f82cd5"

make clean

mkdir -p "${BOOT}"

for file in ${FWFILES[@]}
do
    pull_bin "https://raw.githubusercontent.com/raspberrypi/firmware/${REV}/boot/${file}" "${BOOT}/${file}"
done

dpkg_build files/ vero2-earlysplash-osmc.deb

