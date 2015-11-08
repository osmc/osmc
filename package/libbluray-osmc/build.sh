# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

pull_source "ftp://ftp.videolan.org/pub/videolan/libbluray/0.9.1/libbluray-0.9.1.tar.bz2" "$(pwd)/src"
if [ $? != 0 ]; then echo -e "Error downloading" && exit 1; fi
# Build in native environment
build_in_env "${1}" $(pwd) "libbluray-osmc"
build_return=$?
if [ $build_return == 99 ]
then
	echo -e "Building libbluray"
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
        echo "Depends: ${1}-libbluray-osmc (=${VERSION_NUM}), libfreetype6, libxml2, liblzma5, libpng12-0, libfontconfig1" >> files-dev/DEBIAN/control
	update_sources
	handle_dep "libfreetype6-dev"
	handle_dep "libxml2-dev"
	handle_dep "liblzma-dev"
	handle_dep "libpng12-dev"
	handle_dep "libfontconfig1-dev"
	handle_dep "pkg-config"
	echo "Package: ${1}-libbluray-osmc" >> files/DEBIAN/control && echo "Package: ${1}-libbluray-dev-osmc" >> files-dev/DEBIAN/control
	pushd src/libbluray-*
	./configure --prefix=/usr/osmc --disable-bdjava --enable-udf --with-freetype --with-fontconfig --with-libxml2
	$BUILD
	make install DESTDIR=${out}
	if [ $? != 0 ]; then echo "Error occured during build" && exit 1; fi
	strip_files "${out}"
	popd
	mkdir -p files-dev/usr/osmc
	mv files/usr/osmc/include  files-dev/usr/osmc
	fix_arch_ctl "files/DEBIAN/control"
	fix_arch_ctl "files-dev/DEBIAN/control"
	dpkg_build files ${1}-libbluray-osmc.deb
	dpkg_build files-dev ${1}-libbluray-dev-osmc.deb
	build_return=$?
fi
teardown_env "${1}"
exit $build_return
