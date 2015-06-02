# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

# Build in native environment
build_in_env "${1}" $(pwd) "vero1-wifi-osmc"
build_return=$?
if [ $build_return == 99 ]
then
	echo -e "Building vero1-wifi-osmc"
	out=$(pwd)/files
	make clean
	mkdir -p ${out}/usr/bin
	pushd src
	gcc brcm_patchram_plus.c -o ${out}/usr/bin/brcm_patchram_plus
	if [ $? != 0 ]; then echo "Error occured during build" && exit 1; fi
	strip_files "${out}"
	popd
	dpkg_build files/ vero1-wifi-osmc.deb
	build_return=$?
fi
teardown_env "${1}"
exit $build_return
