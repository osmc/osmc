# (c) 2014 Sam Nazarko
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
	if [ ! -f /tcver.${1} ]; then echo "Not in expected environment" && exit 1; fi
	out=$(pwd)/files
	make clean
	sed '/Package/d' -i files/DEBIAN/control
	sed '/Package/d' -i files-dev/DEBIAN/control
	sed '/Depends/d' -i files-dev/DEBIAN/control
	update_sources
	handle_dep "autoconf"
	handle_dep "libtool"
	test $1 == gen && echo "Package: libshairplay-osmc" >> files/DEBIAN/control && echo "Package: libshairplay-dev-osmc" >> files-dev/DEBIAN/control && echo "Depends: libshairplay-osmc" >> files-dev/DEBIAN/control
	test $1 == rbp && echo "Package: rbp-libshairplay-osmc" >> files/DEBIAN/control && echo "Package: rbp-libshairplay-dev-osmc" >> files-dev/DEBIAN/control && echo "Depends: rbp-libshairplay-osmc" >> files-dev/DEBIAN/control
test $1 == rbp && echo "Package: armv7-libshairplay-osmc" >> files/DEBIAN/control && echo "Package: armv7-libshairplay-dev-osmc" >> files-dev/DEBIAN/control && echo "Depends: armv7-libshairplay-osmc" >> files-dev/DEBIAN/control
	pushd src/shairplay*
	install_patch "../../patches" "all"
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
