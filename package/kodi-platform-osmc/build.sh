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
	echo "Package: ${1}-kodi-platform-osmc" >> files/DEBIAN/control && echo "Package: ${1}-kodi-platform-dev-osmc" >> files-dev/DEBIAN/control && echo "Depends: ${1}-kodi-platform-osmc" >> files-dev/DEBIAN/control
	pushd src/kodi-platform-*
	cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr .
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
