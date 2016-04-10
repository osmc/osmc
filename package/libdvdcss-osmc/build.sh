# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

pull_source "https://github.com/osmc/libdvdcss/archive/2f12236bc1c92f73c21e973363f79eb300de603f.tar.gz" "$(pwd)/src"
if [ $? != 0 ]; then echo -e "Error downloading" && exit 1; fi
# Build in native environment
build_in_env "${1}" $(pwd) "libdvdcss-osmc"
build_return=$?
if [ $build_return == 99 ]
then
	echo -e "Building libdvdcss"
	out=$(pwd)/files
	if [ -d files/usr ]; then rm -rf files/usr; fi
	if [ -d files-dev/usr ]; then rm -rf files-dev/usr; fi
	sed '/Package/d' -i files/DEBIAN/control
	sed '/Package/d' -i files-dev/DEBIAN/control
	sed '/Depends/d' -i files-dev/DEBIAN/control
        sed '/Version/d' -i files-dev/DEBIAN/control
        VERSION_DEV=$(grep Version ${out}/DEBIAN/control)
        VERSION_NUM=$(echo $VERSION_DEV | awk {'print $2'})
        echo $VERSION_DEV >> files-dev/DEBIAN/control
        echo "Depends: ${1}-libdvdcss-osmc (=${VERSION_NUM})" >> files-dev/DEBIAN/control
	update_sources
	handle_dep "autoconf"
	handle_dep "automake"
	handle_dep "libtool"
	echo "Package: ${1}-libdvdcss-osmc" >> files/DEBIAN/control && echo "Package: ${1}-libdvdcss-dev-osmc" >> files-dev/DEBIAN/control
	pushd src/libdvdcss-*
	autoreconf -i
	./configure --prefix=/usr/osmc --disable-shared --enable-static --with-pic
	$BUILD
	make install DESTDIR=${out}
	if [ $? != 0 ]; then echo "Error occured during build" && exit 1; fi
	strip_files "${out}"
	popd
	mkdir -p files-dev/usr/osmc
	mv files/usr/osmc/include  files-dev/usr/osmc
	fix_arch_ctl "files/DEBIAN/control"
	fix_arch_ctl "files-dev/DEBIAN/control"
	dpkg_build files ${1}-libdvdcss-osmc.deb
	dpkg_build files-dev ${1}-libdvdcss-dev-osmc.deb
	build_return=$?
fi
teardown_env "${1}"
exit $build_return
