# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

pull_source "https://github.com/osmc/eventlircd/archive/36bf969f5a18d57a7792d1fb5d1f9acc5929ad2e.tar.gz" "$(pwd)/src"
if [ $? != 0 ]; then echo -e "Error fetching eventlircd source" && exit 1; fi
# Build in native environment
build_in_env "${1}" $(pwd) "eventlircd-osmc"
build_return=$?
if [ $build_return == 99 ]
then
	echo -e "Building package eventlircd"
	out=$(pwd)/files
	make clean
	update_sources
	handle_dep "autoconf"
	handle_dep "automake"
	handle_dep "gawk"
	handle_dep "libudev-dev"
	handle_dep "pkg-config"
	sed '/Package/d' -i files/DEBIAN/control
	sed '/Depends/d' -i files/DEBIAN/control
	echo "Package: ${1}-eventlircd-osmc" >> files/DEBIAN/control
	echo "Depends: ${1}-lirc-osmc, udev" >> files/DEBIAN/control
	pushd src/eventlircd*
	sed -i 's/\s\-Werror//' configure.ac # Disable warnings being errors
	autoreconf -i
	./configure --prefix=/usr --sysconfdir=/etc --with-lircd-socket=/run/lirc/lircd --with-evmap-dir=/etc/eventlircd.d --with-udev-dir=/lib/udev
	if [ $? != 0 ]; then echo -e "Configure failed!" && umount /proc/ > /dev/null 2>&1 && exit 1; fi
	$BUILD
	if [ $? != 0 ]; then echo -e "Build failed!" && exit 1; fi
	make install DESTDIR=${out}
	popd
	strip_files "${out}"
	fix_arch_ctl "files/DEBIAN/control"
	dpkg_build files/ ${1}-eventlircd-osmc.deb
	build_return=$?
fi
teardown_env "${1}"
exit $build_return
