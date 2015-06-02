# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

pull_source "https://github.com/osmc/vero-bootloader/archive/master.tar.gz" "$(pwd)/src"
if [ $? != 0 ]; then echo -e "Error downloading" && exit 1; fi
# Build in native environment
build_in_env "${1}" $(pwd) "vero1-bootloader-osmc"
build_return=$?
if [ $build_return == 99 ]
then
	echo -e "Building package vero-bootloader-osmc"
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
