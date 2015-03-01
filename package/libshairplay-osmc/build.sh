# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

pull_source "http://psg.mtu.edu/pub/xbmc/build-deps/sources/shairplay-139d5ef.tar.bz2" "$(pwd)/src"
if [ $? != 0 ]; then echo -e "Error downloading" && exit 1; fi
# Build in native environment
build_in_env "${1}" $(pwd) "libshairplay-osmc"
if [ $? == 0 ]
then
	echo -e "Building libshairplay"
	out=$(pwd)/files
	make clean
	sed '/Package/d' -i files/DEBIAN/control
	sed '/Package/d' -i files-dev/DEBIAN/control
	sed '/Depends/d' -i files-dev/DEBIAN/control
	update_sources
	handle_dep "autoconf"
	handle_dep "libtool"
	handle_dep "libltdl-dev"
	echo "Package: ${1}-libshairplay-osmc" >> files/DEBIAN/control && echo "Package: ${1}-libshairplay-dev-osmc" >> files-dev/DEBIAN/control && echo "Depends: ${1}-libshairplay-osmc" >> files-dev/DEBIAN/control
	pushd src/shairplay*
	./autogen.sh
	./configure --prefix=/usr
	$BUILD
	make install DESTDIR=${out}
	if [ $? != 0 ]; then echo "Error occured during build" && exit 1; fi
	strip_files "${out}"
	popd
	mkdir -p files-dev/usr
	mv files/usr/include  files-dev/usr/
	fix_arch_ctl "files/DEBIAN/control"
	fix_arch_ctl "files-dev/DEBIAN/control"
	dpkg -b files/ libshairplay-osmc.deb
	dpkg -b files-dev libshairplay-dev-osmc.deb
fi
teardown_env "${1}"
