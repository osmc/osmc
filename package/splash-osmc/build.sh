# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

# Build in native environment
build_in_env "${1}" $(pwd) "splash-osmc"
if [ $? == 0 ]
then
	echo -e "Building splash for OSMC"
	out=$(pwd)/files
	make clean
	sed '/Package/d' -i files/DEBIAN/control
	echo "Package: ${1}-splash-osmc" >> files/DEBIAN/control
	echo "Depends: fbset" >> files/DEBIAN/control
	update_sources
	handle_dep "libpng12-dev"
	pushd src
	$BUILD
	if [ $? != 0 ]; then echo "Error occured during build" && exit 1; fi
	mkdir -p ${out}/usr/bin
	cp -ar ply-image ${out}/usr/bin
	cp -ar splash.png ${out}/usr
	cp -ar splash_sad.png ${out}/usr
	popd
	fix_arch_ctl "files/DEBIAN/control"
	dpkg -b files splash-osmc.deb
fi
teardown_env "${1}"
