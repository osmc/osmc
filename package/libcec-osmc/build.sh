# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

pull_source "https://github.com/Pulse-Eight/libcec/archive/29d82c80bcc62be2878a9ac080de7eb286c4beb9.tar.gz" "$(pwd)/src"
if [ $? != 0 ]; then echo -e "Error downloading" && exit 1; fi
# Build in native environment
build_in_env "${1}" $(pwd) "libcec-osmc"
build_return=$?
if [ $build_return == 99 ]
then
	echo -e "Building libcec"
	out=$(pwd)/files
	make clean
	sed '/Package/d' -i files/DEBIAN/control
	sed '/Package/d' -i files-dev/DEBIAN/control
	sed '/Depends/d' -i files-dev/DEBIAN/control
        sed '/Version/d' -i files-dev/DEBIAN/control
        VERSION_DEV=$(grep Version ${out}/DEBIAN/control)
        VERSION_NUM=$(echo $VERSION_DEV | awk {'print $2'})
        echo $VERSION_DEV >> files-dev/DEBIAN/control
        echo "Depends: ${1}-libcec-osmc (=${VERSION_NUM})" >> files-dev/DEBIAN/control
	update_sources
	handle_dep "libusb-dev"
	handle_dep "libudev-dev"
	handle_dep "autoconf"
	handle_dep "libtool"
	handle_dep "pkg-config"
	handle_dep "python3-dev"
	handle_dep "swig"
	handle_dep "cmake"
	handle_dep "libp8-platform-dev"
	echo "Package: ${1}-libcec-osmc" >> files/DEBIAN/control && echo "Package: ${1}-libcec-dev-osmc" >> files-dev/DEBIAN/control >> files-dev/DEBIAN/control
	pushd src/libcec*
	install_patch "../../patches" "all"
	if [ "$1" == "rbp2" ]; then install_patch "../../patches" "rbp" && PLATFORM="-DHAVE_LINUX_API=1"; fi
	if [ "$1" == "vero3" ]; then install_patch "../../patches" "vero3" && PLATFORM="-DHAVE_AOCEC_API=1"; fi
        if [ "$1" == "vero5" ]; then install_patch "../../patches" "vero5" && PLATFORM="-DHAVE_AOCEC_API=1"; fi
	cmake -DCMAKE_INSTALL_PREFIX=/usr/osmc -DCMAKE_INSTALL_LIBDIR=/usr/osmc/lib -DCMAKE_INSTALL_LIBDIR_NOARCH=/usr/osmc/lib -DBUILD_SHARED_LIBS=1 -DCMAKE_INSTALL_RPATH=/usr/osmc/lib $PLATFORM .
	$BUILD
	make install DESTDIR=${out}
	if [ $? != 0 ]; then echo "Error occured during build" && exit 1; fi
	strip_files "${out}"
	popd
	mkdir -p files-dev/usr/osmc
	mv files/usr/osmc/include  files-dev/usr/osmc
	fix_arch_ctl "files/DEBIAN/control"
	fix_arch_ctl "files-dev/DEBIAN/control"
	dpkg_build files/ ${1}-libcec-osmc.deb
	dpkg_build files-dev ${1}-libcec-dev-osmc.deb
	build_return=$?
fi
teardown_env "${1}"
exit $build_return
