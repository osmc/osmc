# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh
pull_source "http://pkgs.fedoraproject.org/repo/pkgs/lirc/lirc-0.9.0.tar.bz2/b232aef26f23fe33ea8305d276637086/lirc-0.9.0.tar.bz2" "$(pwd)/src"
if [ $? != 0 ]; then echo -e "Error downloading" && exit 1; fi
# Build in native environment
build_in_env "${1}" $(pwd) "lirc-osmc"
build_return=$?
if [ $build_return == 99 ]
then
	echo -e "Building LIRC"
	out=$(pwd)/files
	make clean
	sed '/Package/d' -i files/DEBIAN/control
	update_sources
	handle_dep "libusb-dev"
	handle_dep "autoconf"
	echo "Package: ${1}-lirc-osmc" >> files/DEBIAN/control
	pushd src/lirc-*
	install_patch "../../patches" "all"
	./configure --prefix=/usr --without-x --with-driver=userspace
	$BUILD
	make install DESTDIR=${out}
	if [ $? != 0 ]; then echo "Error occured during build" && exit 1; fi
	strip_files "${out}"
	popd
	fix_arch_ctl "files/DEBIAN/control"
	dpkg_build files/ ${1}-lirc-osmc.deb
	build_return=$?
fi
teardown_env "${1}"
exit $build_return
