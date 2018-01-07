# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh
VERSION="1.35"
pull_source "git://git.kernel.org/pub/scm/network/connman/connman.git" "$(pwd)/src" "${VERSION}"
if [ $? != 0 ]; then echo -e "Error fetching connman source" && exit 1; fi
# Build in native environment
build_in_env "${1}" $(pwd) "connman-osmc"
build_return=$?
if [ $build_return == 99 ]
then
	echo -e "Building package connman"
	out=$(pwd)/files
	make clean
	update_sources
	handle_dep "xtables-addons-source"
	handle_dep "libreadline-dev"
	handle_dep "libdbus-1-dev"
	handle_dep "wpasupplicant"
	handle_dep "iptables"
	handle_dep "libgnutls28-dev"
	handle_dep "libglib2.0-dev"
	handle_dep "openvpn"
	handle_dep "autoconf"
	sed '/Package/d' -i files/DEBIAN/control
	echo "Package: ${1}-connman-osmc" >> files/DEBIAN/control
	pushd src
	install_patch "../patches" "all"
	aclocal
	autoreconf -vif .
	./configure --prefix=/usr --sysconfdir=/etc --localstatedir=/var --enable-openvpn
	if [ $? != 0 ]; then echo -e "Configure failed!" && umount /proc/ > /dev/null 2>&1 && exit 1; fi
	$BUILD
	if [ $? != 0 ]; then echo -e "Build failed!" && exit 1; fi
	make install DESTDIR=${out}
	mkdir -p ${out}/etc/dbus-1/system.d
	mkdir -p ${out}/usr/share/polkit-1/actions
	cp -ar src/connman-dbus-osmc.conf ${out}/etc/dbus-1/system.d/connman-dbus.conf
	cp -ar plugins/polkit.policy ${out}/usr/share/polkit-1/actions/net.connman.policy
	popd
	strip_files "${out}"
	fix_arch_ctl "files/DEBIAN/control"
	dpkg_build files/ ${1}-connman-osmc.deb
	build_return=$?
fi
teardown_env "${1}"
exit $build_return
