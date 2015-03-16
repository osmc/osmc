# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

# Build in native environment
build_in_env "${1}" $(pwd) "vero1-wifi-osmc"
if [ $? == 0 ]
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
	dpkg -b files/ vero1-wifi-osmc.deb
fi
teardown_env "${1}"
