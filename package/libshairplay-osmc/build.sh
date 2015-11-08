# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

pull_source "https://github.com/juhovh/shairplay/archive/0f41ade2678f374aa8446d127d6aa9d5a3d428da.tar.gz" "$(pwd)/src"
if [ $? != 0 ]; then echo -e "Error downloading" && exit 1; fi
# Build in native environment
build_in_env "${1}" $(pwd) "libshairplay-osmc"
build_return=$?
if [ $build_return == 99 ]
then
	echo -e "Building libshairplay"
	out=$(pwd)/files
	make clean
	sed '/Package/d' -i files/DEBIAN/control
	sed '/Package/d' -i files-dev/DEBIAN/control
	sed '/Depends/d' -i files-dev/DEBIAN/control
        sed '/Version/d' -i files-dev/DEBIAN/control
        VERSION_DEV=$(grep Version ${out}/DEBIAN/control)
        VERSION_NUM=$(echo $VERSION_DEV | awk {'print $2'})
        echo $VERSION_DEV >> files-dev/DEBIAN/control
        echo "Depends: ${1}-libshairplay-osmc (=${VERSION_NUM})" >> files-dev/DEBIAN/control
	update_sources
	handle_dep "autoconf"
	handle_dep "libtool"
	handle_dep "libltdl-dev"
	handle_dep "automake"
	echo "Package: ${1}-libshairplay-osmc" >> files/DEBIAN/control && echo "Package: ${1}-libshairplay-dev-osmc" >> files-dev/DEBIAN/control
	pushd src/shairplay*
	./autogen.sh
	./configure --prefix=/usr/osmc
	$BUILD
	make install DESTDIR=${out}
	if [ $? != 0 ]; then echo "Error occured during build" && exit 1; fi
	strip_files "${out}"
	popd
	mkdir -p files-dev/usr/osmc
	mv files/usr/osmc/include  files-dev/usr/osmc
	fix_arch_ctl "files/DEBIAN/control"
	fix_arch_ctl "files-dev/DEBIAN/control"
	dpkg_build files/ ${1}-libshairplay-osmc.deb
	dpkg_build files-dev ${1}-libshairplay-dev-osmc.deb
	build_return=$?
fi
teardown_env "${1}"
exit $build_return
