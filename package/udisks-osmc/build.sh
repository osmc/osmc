# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

VERSION="fe132af7f85fd03a353eec1ec5f8cd8f6a72e191"
pull_source "https://github.com/osmc/udisks-osmc/archive/${VERSION}.tar.gz" "$(pwd)/src"
if [ $? != 0 ]; then echo -e "Error fetching udisks source" && exit 1; fi
# Build in native environment
build_in_env "${1}" $(pwd) "udisks-osmc"
build_return=$?
if [ $build_return == 99 ]
then
	echo -e "Building package udisks"
	out=$(pwd)/files
	make clean
	update_sources
	handle_dep "autotools-dev"
	handle_dep "pkg-config"
	#handle_dep "xsltproc"
	handle_dep "libdbus-glib-1-dev"
	handle_dep "libglib2.0-dev"
	handle_dep "libdbus-1-dev"
	handle_dep "libgudev-1.0-dev"
	handle_dep "libpolkit-gobject-1-dev"
	handle_dep "libparted0-dev"
	handle_dep "libdevmapper-dev"
	handle_dep "liblvm2-dev"
	handle_dep "libatasmart-dev"
	handle_dep "libsgutils2-dev"
	handle_dep "zlib1g-dev"
	handle_dep "libudev-dev"
	handle_dep "uuid-dev"
	handle_dep "libselinux1-dev"
	handle_dep "intltool"
	sed '/^Package/d' -i files/DEBIAN/control
	echo "Package: ${1}-udisks-osmc" >> files/DEBIAN/control
	pushd src/udisks-osmc-${VERSION}
    	install_patch "../../patches" "all"
	./configure --prefix=/usr --enable-man-pages=no --with-systemdsystemunitdir=/lib/systemd/system --disable-dmmp --enable-lvm2 --libexecdir=/usr/lib/udisks --sysconfdir=/etc
	if [ $? != 0 ]; then echo -e "Configure failed!" && umount /proc/ > /dev/null 2>&1 && exit 1; fi
	$BUILD
	if [ $? != 0 ]; then echo -e "Build failed!" && exit 1; fi
	make install DESTDIR=${out}
	popd
	strip_files "${out}"
	fix_arch_ctl "files/DEBIAN/control"
	dpkg_build files/ ${1}-udisks-osmc.deb
	build_return=$?
fi
teardown_env "${1}"
exit $build_return
