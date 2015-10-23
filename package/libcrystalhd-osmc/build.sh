# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

pull_source "https://github.com/osmc/crystalhd/archive/2fe130c68d83ce57322f8a36fdacb40f201d0b67.zip" "$(pwd)/src"
if [ $? != 0 ]; then echo -e "Error downloading" && exit 1; fi
# Build in native environment
build_in_env "${1}" $(pwd) "libcrystalhd-osmc"
build_return=$?
if [ $build_return == 99 ]
then
	echo -e "Building libcrystalhd"
	out=$(pwd)/files
	if [ -d files/usr ]; then rm -rf files/usr; fi
	if [ -d files-dev/usr ]; then rm -rf files-dev/usr; fi
	sed '/Package/d' -i files/DEBIAN/control
	sed '/Package/d' -i files-dev/DEBIAN/control
	sed '/Depends/d' -i files-dev/DEBIAN/control
	update_sources
	echo "Package: ${1}-libcrystalhd-osmc" >> files/DEBIAN/control && echo "Package: ${1}-libcrystalhd-dev-osmc" >> files-dev/DEBIAN/control && echo "Depends: ${1}-libcrystalhd-osmc" >> files-dev/DEBIAN/control
	pushd src/crystalhd*/linux_lib/libcrystalhd
	$BUILD
	make install DESTDIR=${out}
	if [ $? != 0 ]; then echo "Error occured during build" && exit 1; fi
	strip_files "${out}"
	popd
	mkdir -p ${out}/lib/firmware
	mkdir -p files-dev/usr
	mv files/usr/include  files-dev/usr/
	fix_arch_ctl "files/DEBIAN/control"
	fix_arch_ctl "files-dev/DEBIAN/control"
	dpkg_build files ${1}-libcrystalhd-osmc.deb
	dpkg_build files-dev ${1}-libcrystalhd-dev-osmc.deb
	build_return=$?
fi
teardown_env "${1}"
exit $build_return
