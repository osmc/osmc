# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh
VERSION="2.84"
pull_source "http://download.transmissionbt.com/files/transmission-${VERSION}.tar.xz" "$(pwd)/src"
if [ $? != 0 ]; then echo -e "Error downloading" && exit 1; fi
# Build in native environment
build_in_env "${1}" $(pwd) "transmission-app-osmc"
build_return=$?
if [ $build_return == 99 ]
then
	echo -e "Building Transmission"
	out=$(pwd)/files
	sed '/Package/d' -i files/DEBIAN/control
	rm -f files/etc/osmc/apps.d/*transmission-app-osmc
	update_sources
	handle_dep "autoconf"
	handle_dep "libtool"
	handle_dep "pkg-config"
	handle_dep "intltool"
	handle_dep "libssl-dev"
	handle_dep "zlib1g-dev"
	handle_dep "libnatpmp-dev"
	handle_dep "libcurl4-gnutls-dev"
	handle_dep "libglib2.0-dev"
	handle_dep "libevent-dev"
	handle_dep "libminiupnpc-dev"
	handle_dep "libsystemd-daemon-dev"
	mkdir -p files/etc/osmc/apps.d
	echo "Package: ${1}-transmission-app-osmc" >> files/DEBIAN/control && APP_FILE="files/etc/osmc/apps.d/${1}-transmission-app-osmc"
    echo -e "Transmission Client\ntransmission.service" > $APP_FILE
    pushd src/transmission*
    ./configure --prefix=/usr --without-inotify --without-gtk --with-systemd-daemon --enable-lightweight CFLAGS="-O3 -fomit-frame-pointer"
    $BUILD
    make install DESTDIR=${out}
	if [ $? != 0 ]; then echo "Error occured during build" && exit 1; fi
	strip_files "${out}"
	popd
	fix_arch_ctl "files/DEBIAN/control"
	publish_applications_targeted "$(pwd)" "$1" "transmission-app-osmc"
	dpkg_build files/ ${1}-transmission-app-osmc.deb
	build_return=$?
fi
teardown_env "${1}"
exit $build_return
