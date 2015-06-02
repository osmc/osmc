# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

if [ $? == "armv7" ]
then
    pull_source "https://github.com/samnazarko/irqbalanced/archive/master.zip" "$(pwd)/src"
else
    pull_source "https://github.com/Irqbalance/irqbalance/archive/v1.0.8.tar.gz" "$(pwd)/src"
fi
if [ $? != 0 ]; then echo -e "Error downloading" && exit 1; fi
# Build in native environment
build_in_env "${1}" $(pwd) "irqbalance-osmc"
build_return=$?
if [ $build_return == 99 ]
then
	echo -e "Building irqbalance"
	out=$(pwd)/files
	make clean
	sed '/Package/d' -i files/DEBIAN/control
	update_sources
	handle_dep "autoconf"
	handle_dep "libtool"
	handle_dep "pkg-config"
	echo "Package: ${1}-irqbalance-osmc" >> files/DEBIAN/control
	pushd src/irqbalance*
	./autogen.sh
	./configure --prefix=/usr
	$BUILD
	make install DESTDIR=${out}
	if [ $? != 0 ]; then echo "Error occured during build" && exit 1; fi
	strip_files "${out}"
	popd
	fix_arch_ctl "files/DEBIAN/control"
	dpkg_build files/ ${1}-irqbalance-osmc.deb
	build_return=$?
fi
teardown_env "${1}"
exit $build_return
