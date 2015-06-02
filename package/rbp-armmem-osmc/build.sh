# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh
# Build in native environment
pull_source "https://github.com/bavison/arm-mem/archive/master.zip" "$(pwd)/src"
build_in_env "${1}" $(pwd) "rbp-armmem-osmc"
build_return=$?
if [ $build_return == 99 ]
then
	echo -e "Building package rbp-armmem"
	out=$(pwd)/files
	sed '/Package/d' -i files/DEBIAN/control
	echo "Package: ${1}-armmem-osmc" >> files/DEBIAN/control
	make clean
	pushd src/arm-mem-*
	make
	if [ $? != 0 ]; then echo "Error occured during build" && exit 1; fi
	strip_libs
	mkdir -p $out/usr/lib
	if [ "$1" == "rbp1" ]
	then
	    cp -ar libarmmem.so $out/usr/lib/libarmmem.so
	    cp -ar libarmmem.a $out/usr/lib/libarmmem.a
	fi
	if [ "$1" == "rbp2" ]
	then
	    cp -ar libarmmem-a7.so $out/usr/lib/libarmmem.so
	    cp -ar libarmmem-a7.a $out/usr/lib/libarmmem.a
	fi
	popd
	fix_arch_ctl "files/DEBIAN/control"
	dpkg_build files/ ${1}-armmem-osmc.deb
	build_return=$?
fi
teardown_env "${1}"
exit $build_return
