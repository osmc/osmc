# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

pull_source "https://github.com/Pulse-Eight/libcec/archive/libcec-3.0.1.tar.gz" "$(pwd)/src"
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
	sed '/Depends/d' -i files/DEBIAN/control
	sed '/Package/d' -i files-dev/DEBIAN/control
	sed '/Depends/d' -i files-dev/DEBIAN/control
        sed '/Version/d' -i files-dev/DEBIAN/control
	echo "Depends: ${1}-mediacenter-osmc (>= 15.2.0-8)" >> files/DEBIAN/control
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
	handle_dep "cmake"
	if [ "$1" == "rbp1" ]; then handle_dep "armv6l-libplatform-dev-osmc"; fi
	if [ "$1" == "rbp2" ] || [ "$1" == "vero" ]; then handle_dep "armv7-libplatform-dev-osmc"; fi
	if [ "$1" == "rbp1" ] || [ "$1" == "rbp2" ]; then handle_dep "rbp-userland-dev-osmc"; fi
	if [ "$1" == "vero" ]; then handle_dep "vero-userland-dev-osmc"; fi
	if [ "$1" == "i386" ]; then handle_dep "i386-libplatform-dev-osmc"; fi
	echo "Package: ${1}-libcec-osmc" >> files/DEBIAN/control && echo "Package: ${1}-libcec-dev-osmc" >> files-dev/DEBIAN/control >> files-dev/DEBIAN/control
	pushd src/libcec*
	install_patch "../../patches" "all"
	if [ "$1" == "rbp1" ] || [ "$1" == "rbp2" ]; then install_patch "../../patches" "rbp" && PLATFORM="-DRPI_INCLUDE_DIR=/opt/vc/include -DRPI_LIB_DIR=/opt/vc/lib"; fi
	if [ "$1" == "vero" ]; then install_patch "../../patches" "vero" && PLATFORM="-DHAVE_IMX_API=1"; fi
	cmake -DCMAKE_INSTALL_PREFIX=/usr/osmc -DCMAKE_INSTALL_LIBDIR=/usr/osmc/lib -DCMAKE_INSTALL_LIBDIR_NOARCH=/usr/osmc/lib -DBUILD_SHARED_LIBS=1 $PLATFORM .
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
