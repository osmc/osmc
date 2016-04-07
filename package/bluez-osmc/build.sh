# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

if [ "$1" == "trans" ]
then
    echo -e "Building transitional package"
    dpkg_build files-trans ${1}-bluez-osmc.deb
    exit 0
fi

VERSION="5.39"
pull_source "https://www.kernel.org/pub/linux/bluetooth/bluez-${VERSION}.tar.xz" "$(pwd)/src"
if [ $? != 0 ]; then echo -e "Error fetching connman source" && exit 1; fi
# Build in native environment
build_in_env "${1}" $(pwd) "bluez-osmc"
build_return=$?
if [ $build_return == 99 ]
then
	echo -e "Building package bluez"
	out=$(pwd)/files
	make clean
	update_sources
	handle_dep "autoconf"
	handle_dep "automake"
	handle_dep "autopoint"
	handle_dep "autotools-dev"
	handle_dep "gettext"
	handle_dep "libbison-dev"
	handle_dep "libcap-ng-dev"
	handle_dep "libdbus-1-dev"
	handle_dep "libdbus-glib-1-dev"
	handle_dep "libglib2.0-dev"
	handle_dep "libical-dev"
	handle_dep "python-dev"
	handle_dep "libreadline-dev"
	handle_dep "${1}-libsqlite-dev-osmc"
	handle_dep "libudev-dev"
	handle_dep "pkg-config"
	sed '/Package/d' -i files/DEBIAN/control
	echo "Package: ${1}-bluez-osmc" >> files/DEBIAN/control
	pushd src/bluez-$VERSION
    	install_patch "../../patches" "all"
	./configure --prefix=/usr --sysconfdir=/etc --localstatedir=/var
	if [ $? != 0 ]; then echo -e "Configure failed!" && umount /proc/ > /dev/null 2>&1 && exit 1; fi
	$BUILD
	if [ $? != 0 ]; then echo -e "Build failed!" && exit 1; fi
	make install DESTDIR=${out}
	popd
	strip_files "${out}"
	fix_arch_ctl "files/DEBIAN/control"
	dpkg_build files/ ${1}-bluez-osmc.deb
	build_return=$?
fi
teardown_env "${1}"
exit $build_return
