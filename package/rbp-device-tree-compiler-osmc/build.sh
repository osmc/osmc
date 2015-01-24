# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh
# Build in native environment
build_in_env "${1}" $(pwd) "rbp-device-tree-compiler-osmc"
if [ $? == 0 ]
then
	echo -e "Building package rbp-device-tree-compiler"
	out=$(pwd)/files
	make clean
	update_sources
	handle_dep "bison"
	handle_dep "flex"
	pushd src/
	$BUILD PREFIX=/usr
	make install DESTDIR=$out
	if [ $? != 0 ]; then echo "Error occured during build" && exit 1; fi
	# This is weird...
	mv $out/root $out/usr
	strip_files "${out}"
	popd
	dpkg -b files/ rbp-device-tree-compiler-osmc.deb
fi
teardown_env "${1}"
