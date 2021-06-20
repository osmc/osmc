# (c) 2014-2020 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh
REV="b09f373ea7dbc6e3ecbcb74d7299f5230cdc6e59"
pull_source "https://github.com/Arkq/bluez-alsa/archive/${REV}.tar.gz" "$(pwd)/src"
if [ $? != 0 ]; then echo -e "Error fetching bluez-alsa source" && exit 1; fi
# Build in native environment
build_in_env "${1}" $(pwd) "bluezalsa-osmc"
build_return=$?
if [ $build_return == 99 ]
then
	echo -e "Building package bluezalsa-osmc"
	out=$(pwd)/files
	make clean
	update_sources
	handle_dep "libasound2-dev"
	handle_dep "libbluetooth-dev"
	handle_dep "libdbus-1-dev"
	handle_dep "libsbc-dev"
	handle_dep "libglib2.0-dev"
	handle_dep "libfdk-aac-dev"
	handle_dep "automake"
	handle_dep "libtool"
	handle_dep "pkg-config"
	sed '/Package/d' -i files/DEBIAN/control
	echo "Package: ${1}-bluezalsa-osmc" >> files/DEBIAN/control
	pushd src/bluez-alsa*
	autoreconf -vif .
	./configure --enable-aac --with-alsaplugindir=/usr/lib/arm-linux-gnueabihf/alsa-lib/ --with-alsaconfdir=/usr/share/alsa/alsa.conf.d
	if [ $? != 0 ]; then echo -e "Configure failed!" && umount /proc/ > /dev/null 2>&1 && exit 1; fi
	$BUILD
	if [ $? != 0 ]; then echo -e "Build failed!" && exit 1; fi
	make install DESTDIR=${out}
	popd
	#strip_files "${out}"
	fix_arch_ctl "files/DEBIAN/control"
	dpkg_build files/ ${1}-bluezalsa-osmc.deb
	build_return=$?
fi
teardown_env "${1}"
exit $build_return
