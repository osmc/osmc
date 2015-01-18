# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

pull_source "http://eventlircd.googlecode.com/svn/trunk/" "$(pwd)/src"
if [ $? != 0 ]; then echo -e "Error fetching eventlircd source" && exit 1; fi
# Build in native environment
build_in_env "${1}" $(pwd) "eventlircd-osmc"
if [ $? == 0 ]
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
	test "$1" == rbp && echo "Package: rbp-eventlircd-osmc" >> files/DEBIAN/control
	pushd src
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
	dpkg -b files/ eventlircd-osmc.deb
fi
teardown_env "${1}"
