# (c) 2014-2020 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh
VERSION="2.1.0-osmc"
pull_source "https://github.com/Arkq/bluez-alsa/archive/49ad348808a15485aa7cb2df0a4d13654cc0cee3.tar.gz" "$(pwd)/src"
if [ $? != 0 ]; then echo -e "Error fetching bluez-alsa source" && exit 1; fi
# Build in native environment
build_in_env "${1}" $(pwd) "bluez-alsa-app-osmc"
build_return=$?
if [ $build_return == 99 ]
then
	echo -e "Building package bluez-alsa"
	out=$(pwd)/files
	make clean
	update_sources
	handle_dep "libasound2-dev"
	handle_dep "libbluetooth-dev"
	handle_dep "libdbus-1-dev"
	handle_dep "libsbc-dev"
	handle_dep "libglib2.0-dev"
	handle_dep "automake"
	handle_dep "libtool"
	handle_dep "pkg-config"
	sed '/Package/d' -i files/DEBIAN/control
	echo "Package: ${1}-bluez-alsa-app-osmc" >> files/DEBIAN/control
	pushd src
	SOURCE=$(ls)
	pushd $SOURCE
	install_patch "../../patches" "all"
	# aclocal
	autoreconf --install
	mkdir build && pushd build
	../configure --enable-aac --with-alsaplugindir=/usr/lib/arm-linux-gnueabihf/alsa-lib/ --with-alsaconfdir=/usr/share/alsa/alsa.conf.d
	if [ $? != 0 ]; then echo -e "Configure failed!" && umount /proc/ > /dev/null 2>&1 && exit 1; fi
	$BUILD
	if [ $? != 0 ]; then echo -e "Build failed!" && exit 1; fi
	make install DESTDIR=${out}
	#mkdir -p ${out}/etc/dbus-1/system.d
	#mkdir -p ${out}/usr/share/polkit-1/actions
	#cp -ar src/connman-dbus-osmc.conf ${out}/etc/dbus-1/system.d/connman-dbus.conf
	#cp -ar plugins/polkit.policy ${out}/usr/share/polkit-1/actions/net.connman.policy
	popd; popd; popd
	strip_files "${out}"
	fix_arch_ctl "files/DEBIAN/control"
	dpkg_build files/ ${1}-bluez-alsa-app-osmc.deb
	build_return=$?
fi
teardown_env "${1}"
exit $build_return
