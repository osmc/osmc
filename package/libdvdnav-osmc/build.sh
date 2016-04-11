# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

pull_source "https://github.com/osmc/libdvdnav/archive/43b5f81f5fe30bceae3b7cecf2b0ca57fc930dac.tar.gz" "$(pwd)/src"
if [ $? != 0 ]; then echo -e "Error downloading" && exit 1; fi
# Build in native environment
build_in_env "${1}" $(pwd) "libdvdnav-osmc"
build_return=$?
if [ $build_return == 99 ]
then
	echo -e "Building libdvdnav"
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
        echo "Depends: ${1}-libdvdnav-osmc (=${VERSION_NUM})" >> files-dev/DEBIAN/control
	update_sources
	handle_dep "autoconf"
	handle_dep "automake"
	handle_dep "libtool"
	handle_dep "pkg-config"
	echo "Package: ${1}-libdvdnav-osmc" >> files/DEBIAN/control && echo "Package: ${1}-libdvdnav-dev-osmc" >> files-dev/DEBIAN/control
	pushd src/libdvdnav-*
	export CFLAGS="-D_XBMC $CFLAGS"
	autoreconf -i
	./configure --prefix=/usr/osmc
	$BUILD
	make install DESTDIR=${out}
	if [ $? != 0 ]; then echo "Error occured during build" && exit 1; fi
	strip_files "${out}"
	popd
	mkdir -p ${out}/usr/lib/kodi/system/players/VideoPlayer/
	ln -s /usr/osmc/lib/libdvdnav.so ${out}/usr/lib/kodi/system/players/VideoPlayer/libdvdnav-arm.so
	mkdir -p files-dev/usr/osmc
	mv files/usr/osmc/include  files-dev/usr/osmc
	fix_arch_ctl "files/DEBIAN/control"
	fix_arch_ctl "files-dev/DEBIAN/control"
	dpkg_build files ${1}-libdvdnav-osmc.deb
	dpkg_build files-dev ${1}-libdvdnav-dev-osmc.deb
	build_return=$?
fi
teardown_env "${1}"
exit $build_return
