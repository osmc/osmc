# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

pull_source "https://github.com/osmc/udevil/archive/4f79ba4233bcbe9a2c9b8a799cd9dedba9534eaf.tar.gz" "$(pwd)/src"
if [ $? != 0 ]; then echo -e "Error fetching udevil source" && exit 1; fi
# Build in native environment
build_in_env "${1}" $(pwd) "udevil-osmc"
build_return=$?
if [ $build_return == 99 ]
then
	echo -e "Building package udevil"
	out=$(pwd)/files
	make clean
	update_sources
	handle_dep "libudev-dev"
        handle_dep "libglib2.0-dev"
	handle_dep "intltool"
	handle_dep "gettext"
	sed '/Package/d' -i files/DEBIAN/control
	sed '/Depends/d' -i files/DEBIAN/control
	echo "Package: ${1}-udevil-osmc" >> files/DEBIAN/control
	echo "Depends: ${1}-lirc-osmc, libudev1, libglib2.0-0" >> files/DEBIAN/control
	pushd src/udevil*
        ./configure --prefix=/usr
	if [ $? != 0 ]; then echo -e "Configure failed!" && umount /proc/ > /dev/null 2>&1 && exit 1; fi
	$BUILD
	if [ $? != 0 ]; then echo -e "Build failed!" && exit 1; fi
	make install DESTDIR=${out}
	popd
	strip_files "${out}"
	fix_arch_ctl "files/DEBIAN/control"
	dpkg_build files/ ${1}-udevil-osmc.deb
	build_return=$?
fi
teardown_env "${1}"
exit $build_return
