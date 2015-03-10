# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh
# Build in native environment
if [ $1 == "rbp1" ]; then pull_source "https://github.com/bavison/arm-mem/archive/cd2c8f9202137c79f7afb77ecb87e713a0800d3c.zip" "$(pwd)/src"; fi
if [ $1 == "rbp2" ]; then pull_source "https://github.com/bavison/arm-mem/archive/master.zip" "$(pwd)/src"; fi
build_in_env "${1}" $(pwd) "rbp-armmem-osmc"
if [ $? == 0 ]
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
	cp -ar libarmmem.so $out/usr/lib
	cp -ar libarmmem.a $out/usr/lib
	popd
        fix_arch_ctl "files/DEBIAN/control"
	dpkg -b files/ rbp-armmem-osmc.deb
fi
teardown_env "${1}"
