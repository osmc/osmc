# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

if [ "$1" == "trans" ]
then
    echo -e "Building transitional package"
    dpkg_build files-trans ${1}-udisks-glue-osmc.deb
    exit 0
fi

VERSION="6b114c8ae066e20277dd8f95447d0083b9bc163f"
pull_source "https://github.com/osmc/udisks-glue-osmc/archive/${VERSION}.tar.gz" "$(pwd)/src"
if [ $? != 0 ]; then echo -e "Error fetching udisks source" && exit 1; fi
# Build in native environment
build_in_env "${1}" $(pwd) "udisks-glue-osmc"
build_return=$?
if [ $build_return == 99 ]
then
	echo -e "Building package udisks-glue"
	out=$(pwd)/files
	make clean
	update_sources
	handle_dep "dpkg-dev"
	handle_dep "libconfuse-dev"
	handle_dep "libdbus-glib-1-dev"
	handle_dep "libglib2.0-dev"
	handle_dep "pkg-config"
	sed '/Package/d' -i files/DEBIAN/control
	echo "Package: ${1}-udisks-glue-osmc" >> files/DEBIAN/control
	pushd src/udisks-glue-osmc-${VERSION}
	./autogen.sh
	./configure --prefix=/usr --sysconfdir=/etc
	if [ $? != 0 ]; then echo -e "Configure failed!" && umount /proc/ > /dev/null 2>&1 && exit 1; fi
	$BUILD
	if [ $? != 0 ]; then echo -e "Build failed!" && exit 1; fi
	make install DESTDIR=${out}
	popd
	strip_files "${out}"
	fix_arch_ctl "files/DEBIAN/control"
	dpkg_build files/ ${1}-udisks-glue-osmc.deb
	build_return=$?
fi
teardown_env "${1}"
exit $build_return
