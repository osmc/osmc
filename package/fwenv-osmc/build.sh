# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh
VERSION="2016.01"
pull_source "ftp://ftp.denx.de/pub/u-boot/u-boot-${VERSION}.tar.bz2" "$(pwd)/src"
if [ $? != 0 ]; then echo -e "Error fetching u-boot-tools source" && exit 1; fi
# Build in native environment
build_in_env "${1}" $(pwd) "fwenv-osmc"
build_return=$?
if [ $build_return == 99 ]
then
	echo -e "Building package fwenv-osmc"
	out=$(pwd)/files
	make clean
	update_sources
	sed '/Package/d' -i files/DEBIAN/control
	echo "Package: ${1}-fwenv-osmc" >> files/DEBIAN/control
	pushd src/u-boot-$VERSION
	if [ "$1" == "vero3" ]; then install_patch "../../patches" "vero3"; fi
	make blank_defconfig
	make env
	if [ $? != 0 ]; then echo -e "Build failed!" && exit 1; fi
	pushd tools/env
	ln -s fw_printenv fw_setenv
	popd
	mkdir -p ${out}/usr/bin
	cp -ar tools/env/fw_*env ${out}/usr/bin
	popd
	strip_files "${out}"
	fix_arch_ctl "files/DEBIAN/control"
	dpkg_build files/ ${1}-fwenv-osmc.deb
	build_return=$?
fi
teardown_env "${1}"
exit $build_return
