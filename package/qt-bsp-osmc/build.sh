# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

pull_source "http://download.qt.io/official_releases/qt/5.5/5.5.0/single/qt-everywhere-opensource-src-5.5.0.tar.gz" "$(pwd)/src"
if [ $? != 0 ]; then echo -e "Error downloading" && exit 1; fi
# Build in native environment
build_in_env "${1}" $(pwd) "qt-bsp-osmc"
build_return=$?
if [ $build_return == 99 ]
then
	echo -e "Building Qt BSP for OSMC"
	out=$(pwd)/files
	make clean
        sed '/Package/d' -i files/DEBIAN/control
        sed '/Package/d' -i files-dev/DEBIAN/control
        sed '/Depends/d' -i files-dev/DEBIAN/control
        sed '/Version/d' -i files-dev/DEBIAN/control
        VERSION_DEV=$(grep Version ${out}/DEBIAN/control)
        VERSION_NUM=$(echo $VERSION_DEV | awk {'print $2'})
        echo $VERSION_DEV >> files-dev/DEBIAN/control
        echo "Depends: ${1}-qt-bsp-osmc (=${VERSION_NUM})" >> files-dev/DEBIAN/control
	update_sources
	handle_dep "libudev-dev"
	handle_dep "libdbus-1-dev"
	handle_dep "libssl-dev"
	handle_dep "libasound2-dev"
	handle_dep "libfreetype6-dev"
	handle_dep "libpng12-dev"
	handle_dep "libjpeg62-turbo-dev"
	handle_dep "zlib1g-dev"
	handle_dep "libsqlite3-dev"
	echo "Package: ${1}-qt-bsp-osmc" >> files/DEBIAN/control && echo "Package: ${1}-qt-bsp-dev-osmc" >> files-dev/DEBIAN/control
	if [ "$1" == "rbp1" ] || [ "$1" == "rbp2" ]; then handle_dep "rbp-userland-dev-osmc"; fi
	if [ "$1" == "vero" ]; then handle_dep "vero-userland-dev-osmc"; fi
	pushd src/qt-everywhere-opensource*
	install_patch "../../patches" "all"
	if [ "$1" == "rbp1" ] || [ "$1" == "rbp2" ]; then install_patch "../../patches" "rbp"; fi
	install_patch "../../patches" "$1"
	if [ "$1" == "rbp1" ] || [ "$1" == "rbp2" ]; then PLATFORM="-no-directfb -no-linuxfb -eglfs -no-xcb -device linux-rasp-pi-g++ -device-option CROSS_COMPILE=/usr/bin/ -sysroot /"; fi
        if [ "$1" == "vero" ]; then PLATFORM="-no-directfb -no-linuxfb -eglfs -no-xcb -device linux-imx6-g++ -device-option CROSS_COMPILE=/usr/bin/ -sysroot /"; fi
	./configure --prefix=/usr -release -opensource -confirm-license -c++11 -no-largefile -no-qml-debug -system-zlib \
	-no-journald -system-libpng -system-freetype -system-libjpeg -system-sqlite -openssl -no-pulseaudio -alsa \
	-no-sql-db2 -no-sql-ibase -no-sql-mysql -no-sql-oci -no-sql-odbc -no-sql-psql -no-sql-sqlite2 -no-sql-tds -nomake examples -reduce-exports $PLATFORM
	$BUILD
	make install INSTALL_ROOT=${out}
	if [ $? != 0 ]; then echo "Error occured during build" && exit 1; fi
	strip_files "${out}"
	popd
	mkdir -p files-dev/usr
	mv files/usr/include  files-dev/usr/
	fix_arch_ctl "files/DEBIAN/control"
	fix_arch_ctl "files-dev/DEBIAN/control"
	dpkg_build files/ ${1}-qt-bsp-osmc.deb
	dpkg_build files-dev ${1}-qt-bsp-dev-osmc.deb
	build_return=$?
fi
teardown_env "${1}"
exit $build_return
