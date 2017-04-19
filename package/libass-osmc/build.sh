# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

pull_source "https://github.com/libass/libass/archive/8551555c86f50f978f9ddd55a8d20bceb80d92fc.tar.gz" "$(pwd)/src"
if [ $? != 0 ]; then echo -e "Error downloading" && exit 1; fi
# Build in native environment
build_in_env "${1}" $(pwd) "libass-osmc"
build_return=$?
if [ $build_return == 99 ]
then
	echo -e "Building libass"
	out=$(pwd)/files
	if [ -d files/usr ]; then rm -rf files/usr; fi
	if [ -d files-dev/usr ]; then rm -rf files-dev/usr; fi
	sed '/Package/d' -i files/DEBIAN/control
	sed '/Package/d' -i files-dev/DEBIAN/control
	sed '/Depends/d' -i files-dev/DEBIAN/control
	sed '/Depends/d' -i files/DEBIAN/control
        sed '/Version/d' -i files-dev/DEBIAN/control
        VERSION_DEV=$(grep Version ${out}/DEBIAN/control)
        VERSION_NUM=$(echo $VERSION_DEV | awk {'print $2'})
        echo $VERSION_DEV >> files-dev/DEBIAN/control
        echo "Depends: libenca0, libfontconfig1, libfreetype6, libfribidi0, libharfbuzz0b" >> files/DEBIAN/control
        echo "Depends: ${1}-libass-osmc (=${VERSION_NUM})" >> files-dev/DEBIAN/control
	update_sources
	handle_dep "libenca-dev"
	handle_dep "libfontconfig1-dev"
	handle_dep "libfreetype6-dev"
	handle_dep "libfribidi-dev"
	handle_dep "libharfbuzz-dev"
	handle_dep "autoconf"
	handle_dep "automake"
	handle_dep "libtool"
	echo "Package: ${1}-libass-osmc" >> files/DEBIAN/control && echo "Package: ${1}-libass-dev-osmc" >> files-dev/DEBIAN/control
	pushd src/libass-*
	autoreconf -vif
	./configure --prefix=/usr/osmc
	$BUILD
	make install DESTDIR=${out}
	if [ $? != 0 ]; then echo "Error occured during build" && exit 1; fi
	strip_files "${out}"
	popd
	mkdir -p files-dev/usr/osmc
	mv files/usr/osmc/include  files-dev/usr/osmc
	fix_arch_ctl "files/DEBIAN/control"
	fix_arch_ctl "files-dev/DEBIAN/control"
	dpkg_build files ${1}-libass-osmc.deb
	dpkg_build files-dev ${1}-libass-dev-osmc.deb
	build_return=$?
fi
teardown_env "${1}"
exit $build_return
