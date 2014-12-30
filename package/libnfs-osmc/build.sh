# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

pull_source "https://sites.google.com/site/libnfstarballs/li/libnfs-1.9.6.tar.gz?attredirects=0&d=1" "$(pwd)/src"
if [ $? != 0 ]; then echo -e "Error downloading" && exit 1; fi
# Build in native environment
build_in_env "${1}" $(pwd) "libnfs-osmc"
if [ $? == 0 ]
then
	echo -e "Building libnfs"
	if [ ! -f /tcver.${1} ]; then echo "Not in expected environment" && exit 1; fi
	out=$(pwd)/files
	if [ -d files/usr ]; then rm -rf files/usr; fi
	if [ -d files-dev/usr ]; then rm -rf files-dev/usr; fi
	sed '/Package/d' -i files/DEBIAN/control
	sed '/Package/d' -i files-dev/DEBIAN/control
	sed '/Depends/d' -i files-dev/DEBIAN/control
	update_sources
	handle_dep "autoconf"
	handle_dep "libtool"
	test "$1" == gen && echo "Package: libnfs-osmc" >> files/DEBIAN/control && echo "Package: libnfs-dev-osmc" >> files-dev/DEBIAN/control && echo "Depends: libnfs-osmc" >> files-dev/DEBIAN/control
	test "$1" == rbp && echo "Package: rbp-libnfs-osmc" >> files/DEBIAN/control && echo "Package: rbp-libnfs-dev-osmc" >> files-dev/DEBIAN/control && echo "Depends: rbp-libnfs-osmc" >> files-dev/DEBIAN/control
	test "$1" == armv7 && echo "Package: armv7-libnfs-osmc" >> files/DEBIAN/control && echo "Package: armv7-libnfs-dev-osmc" >> files-dev/DEBIAN/control && echo "Depends: armv7-libnfs-osmc" >> files-dev/DEBIAN/control
	pushd src
	./bootstrap
	./configure --prefix=/usr
	$BUILD
	make install DESTDIR=${out}
	if [ $? != 0 ]; then echo "Error occured during build" && exit 1; fi
	strip_files "${out}"
	popd
	mkdir -p files-dev/usr
	mv files/usr/include  files-dev/usr/
	fix_arch_ctl "files/DEBIAN/control"
	fix_arch_ctl "files-dev/DEBIAN/control"
	dpkg -b files libnfs-osmc.deb
	dpkg -b files-dev libnfs-dev-osmc.deb
fi
teardown_env "${1}"
