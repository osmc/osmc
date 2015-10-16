# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

pull_source "https://qa.debian.org/watch/sf.php/hd-idle/hd-idle-1.04.tgz" "$(pwd)/src"
if [ $? != 0 ]; then echo -e "Error fetching hd-idle source" && exit 1; fi
# Build in native environment
build_in_env "${1}" $(pwd) "hd-idle-osmc"
build_return=$?
if [ $build_return == 99 ]
then
	echo -e "Building package hdidle"
	out=$(pwd)/files
	make clean
	sed '/Package/d' -i files/DEBIAN/control
	echo "Package: ${1}-hd-idle-osmc" >> files/DEBIAN/control
	pushd src/hd-idle*
	$BUILD
	if [ $? != 0 ]; then echo -e "Build failed!" && exit 1; fi
	make install DESTDIR=${out}
	popd
	strip_files "${out}"
	fix_arch_ctl "files/DEBIAN/control"
	dpkg_build files/ ${1}-hd-idle-osmc.deb
	build_return=$?
fi
teardown_env "${1}"
exit $build_return
