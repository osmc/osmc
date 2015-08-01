# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

pull_source "https://github.com/xbmc/kodi-platform/archive/054a42f664af3a6740d49759c081d4929a190671.tar.gz" "$(pwd)/src"
if [ $? != 0 ]; then echo -e "Error downloading" && exit 1; fi
# Build in native environment
build_in_env "${1}" $(pwd) "kodi-platform-osmc"
build_return=$?
if [ $build_return == 99 ]
then
	echo -e "Building kodi-platform-osmc"
	out=$(pwd)/files
	if [ -d files/usr ]; then rm -rf files/usr; fi
	if [ -d files-dev/usr ]; then rm -rf files-dev/usr; fi
	sed '/Package/d' -i files/DEBIAN/control
	sed '/Package/d' -i files-dev/DEBIAN/control
	sed '/Depends/d' -i files-dev/DEBIAN/control
	update_sources
	handle_dep "cmake"
	handle_dep "wget"
	handle_dep "libtinyxml-dev"
        if [ "$1" == "rbp2" ] || [ "$1" == "vero" ]; then handle_dep "armv7-libplatform-dev-osmc"; fi
        if [ "$1" == "rbp1" ]; then handle_dep "armv6l-libplatform-dev-osmc"; fi
        if [ "$1" == "atv" ]; then handle_dep "i386-libplatform-dev-osmc"; fi
	# Hack # Hack # Hack
	# This is an unconventional handle_dep
	# We don't have a dev package for Kodi, but we need some bits from it.
	# We don't want all the dependencies it drags in.
	DEP=$(apt-get install -qq --print-uris ${1}-mediacenter-osmc | grep ${1}-mediacenter-osmc | perl -lne "/'(.*?)'/;print \$1" | tail -n 1)
	wget ${DEP} -O $(pwd)/src/mediacenter-osmc.deb # Can't pull_bin in chroot
	if [ $? != 0 ]; then echo "Downloading Kodi package failed"; fi
	dpkg -x $(pwd)/src/mediacenter-osmc.deb $(pwd)/src/kodi-platform-*
	echo "Package: ${1}-kodi-platform-osmc" >> files/DEBIAN/control && echo "Package: ${1}-kodi-platform-dev-osmc" >> files-dev/DEBIAN/control && echo "Depends: ${1}-kodi-platform-osmc" >> files-dev/DEBIAN/control
	pushd src/kodi-platform-*
	# Fix stupid defaults
	sed s:/usr/include/kodi:$(pwd)/usr/include/kodi: -i $(pwd)/usr/lib/kodi/kodi-config.cmake
	sed s:/usr/lib/kodi:$(pwd)/usr/lib/kodi: -i $(pwd)/usr/lib/kodi/kodi-config.cmake
	cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr -Dkodi_DIR=$(pwd)/usr/lib/kodi/ .
	$BUILD
	make install DESTDIR=${out}
	if [ $? != 0 ]; then echo "Error occured during build" && exit 1; fi
	strip_files "${out}"
	popd
	mkdir -p files-dev/usr
	mv files/usr/include  files-dev/usr/
	fix_arch_ctl "files/DEBIAN/control"
	fix_arch_ctl "files-dev/DEBIAN/control"
	dpkg_build files ${1}-kodi-platform-osmc.deb
	dpkg_build files-dev ${1}-kodi-platform-dev-osmc.deb
	build_return=$?
fi
teardown_env "${1}"
exit $build_return
