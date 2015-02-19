# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

pull_source "https://github.com/irqbalance/irqbalance/archive/master.tar.gz" "$(pwd)/src"
if [ $? != 0 ]; then echo -e "Error downloading" && exit 1; fi
# Build in native environment
build_in_env "${1}" $(pwd) "irqbalance-osmc"
if [ $? == 0 ]
then
	echo -e "Building irqbalance"
	out=$(pwd)/files
	make clean
	sed '/Package/d' -i files/DEBIAN/control
	update_sources
	handle_dep "autoconf"
	handle_dep "libtool"
	handle_dep "pkg-config"
	test "$1" == armv7 && echo "Package: armv7-irqbalance-osmc" >> files/DEBIAN/control
	pushd src/irqbalance-master
	./autogen.sh
	./configure --prefix=/usr
	$BUILD
	make install DESTDIR=${out}
	if [ $? != 0 ]; then echo "Error occured during build" && exit 1; fi
	strip_files "${out}"
	popd
	fix_arch_ctl "files/DEBIAN/control"
	dpkg -b files/ irqbalance-osmc.deb
fi
teardown_env "${1}"
