# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh
# Build in native environment
pull_source "https://github.com/bavison/arm-mem" "$(pwd)/src"
build_in_env "${1}" $(pwd) "rbp-armmem-osmc"
if [ $? == 0 ]
then
	echo -e "Building package rbp-armmem"
	out=$(pwd)/files
	make clean
	pushd src
	make
	if [ $? != 0 ]; then echo "Error occured during build" && exit 1; fi
	strip_libs
	mkdir -p $out/usr/lib
	cp -ar libarmmem.so $out/usr/lib
	cp -ar libarmmem.a $out/usr/lib
	popd
	dpkg -b files/ rbp-armmem-osmc.deb
fi
teardown_env "${1}"
