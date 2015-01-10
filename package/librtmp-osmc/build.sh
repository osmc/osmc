# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

pull_source "git://git.ffmpeg.org/rtmpdump" "$(pwd)/src"
if [ $? != 0 ]; then echo -e "Error downloading" && exit 1; fi
# Build in native environment
build_in_env "${1}" $(pwd) "librtmp-osmc"
if [ $? == 0 ]
then
	echo -e "Building librtmp"
	if [ ! -f /tcver.${1} ]; then echo "Not in expected environment" && exit 1; fi
	out=$(pwd)/files
	make clean
	sed '/Package/d' -i files/DEBIAN/control
	sed '/Package/d' -i files-dev/DEBIAN/control
	sed '/Depends/d' -i files-dev/DEBIAN/control
	update_sources
	handle_dep "libssl-dev"
	test "$1" == gen && echo "Package: librtmp-osmc" >> files/DEBIAN/control && echo "Package: librtmp-dev-osmc" >> files-dev/DEBIAN/control && echo "Depends: librtmp-osmc" >> files-dev/DEBIAN/control
	test "$1" == rbp && echo "Package: rbp-librtmp-osmc" >> files/DEBIAN/control && echo "Package: rbp-librtmp-dev-osmc" >> files-dev/DEBIAN/control && echo "Depends: rbp-librtmp-osmc" >> files-dev/DEBIAN/control
	test "$1" == armv7 && echo "Package: armv7-librtmp-osmc" >> files/DEBIAN/control && echo "Package: armv7-librtmp-dev-osmc" >> files-dev/DEBIAN/control && echo "Depends: armv7-librtmp-osmc" >> files-dev/DEBIAN/control
	pushd src
	$BUILD sys=posix
	if [ $? != 0 ]; then echo "Error occured during build" && exit 1; fi
	pushd librtmp
	strip_libs
	mkdir -p $out/usr/lib
	cp -ar librtmp.so.1 $out/usr/lib
	cp -ar librtmp.a $out/usr/lib
	pushd $out/usr/lib
	ln -s librtmp.so.1 librtmp.so
	popd
	popd
	popd
	fix_arch_ctl "files/DEBIAN/control"
	dpkg -b files/ librtmp-osmc.deb
	out=$(pwd)/files-dev
	cd src
	cd librtmp
	mkdir -p $out/usr/include/librtmp
	cp -ar *.h $out/usr/include/librtmp
	cd ../../
	fix_arch_ctl "files-dev/DEBIAN/control"
	dpkg -b files-dev librtmp-dev-osmc.deb
fi
teardown_env "${1}"
