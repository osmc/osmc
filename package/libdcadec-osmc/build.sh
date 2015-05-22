# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

pull_source "https://github.com/foo86/dcadec/archive/master.tar.gz" "$(pwd)/src"
if [ $? != 0 ]; then echo -e "Error downloading" && exit 1; fi
# Build in native environment
build_in_env "${1}" $(pwd) "libdcadec-osmc"
build_return=$?
if [ $build_return == 99 ]
then
	echo -e "Building libdcadec"
	out=$(pwd)/files
	make clean
	sed '/Package/d' -i files/DEBIAN/control
	sed '/Package/d' -i files-dev/DEBIAN/control
	sed '/Depends/d' -i files-dev/DEBIAN/control
	echo "Package: ${1}-libdcadec-osmc" >> files/DEBIAN/control && echo "Package: ${1}-libdcadec-dev-osmc" >> files-dev/DEBIAN/control && echo "Depends: ${1}-libdcadec-osmc" >> files-dev/DEBIAN/control
	pushd src/dcadec*
	$BUILD
	if [ $? != 0 ]; then echo "Error occured during build" && exit 1; fi
	pushd libdcadec
	strip_libs
	mkdir -p ${out}/usr/lib
	cp -ar libdcadec.a ${out}/usr/lib
	pushd ${out}/usr/lib
	popd
	mkdir -p ${out}-dev/usr/include/libdcadec
	cp -ar *.h ${out}-dev/usr/include/libdcadec
	popd
	popd
	fix_arch_ctl "files/DEBIAN/control"
	dpkg -b files/ libdcadec-osmc.deb
	fix_arch_ctl "files-dev/DEBIAN/control"
	dpkg -b files-dev libdcadec-dev-osmc.deb
	build_return=$?
fi
teardown_env "${1}"
exit $build_return
