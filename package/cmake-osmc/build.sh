# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh
VERSION="3.6.2"
pull_source "https://cmake.org/files/v3.6/cmake-${VERSION}.tar.gz" "$(pwd)/src"
if [ $? != 0 ]; then echo -e "Error fetching CMake source" && exit 1; fi
# Build in native environment
build_in_env "${1}" $(pwd) "cmake-osmc"
build_return=$?
if [ $build_return == 99 ]
then
	echo -e "Building package cmake-osmc"
	out=$(pwd)/files
	make clean
	update_sources
	handle_dep "libarchive-dev"
	handle_dep "libjsoncpp-dev"
	handle_dep "liblzma-dev"
	handle_dep "libexpat1-dev"
	handle_dep "libbz2-dev"
	handle_dep "zlib1g-dev"
	handle_dep "libcurl4-openssl-dev"
	sed '/Package/d' -i files/DEBIAN/control
	echo "Package: ${1}-cmake-osmc" >> files/DEBIAN/control
	pushd src/cmake-$VERSION
	./bootstrap --enable-ccache --system-libs --no-qt-gui --prefix=/usr
	if [ $? != 0 ]; then echo -e "Bootstrap failed!" && umount /proc/ > /dev/null 2>&1 && exit 1; fi
	$BUILD
	if [ $? != 0 ]; then echo -e "Build failed!" && exit 1; fi
	make install DESTDIR=${out}
	popd
	strip_files "${out}"
	fix_arch_ctl "files/DEBIAN/control"
	dpkg_build files/ ${1}-cmake-osmc.deb
	build_return=$?
fi
teardown_env "${1}"
exit $build_return
