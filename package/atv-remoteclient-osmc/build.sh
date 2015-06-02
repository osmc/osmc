# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

# Build in native environment
pull_source "https://github.com/samnazarko/atvclient/archive/master.zip" "$(pwd)/src"
build_in_env "${1}" $(pwd) "atv-remoteclient-osmc"
build_return=$?
if [ $build_return == 99 ]
then
	echo -e "Building atvclient"
	out=$(pwd)/files
	make clean
	update_sources
	handle_dep "libusb-dev"
        handle_dep "autoconf"
        handle_dep "pkg-config"
	pushd src/atvclient*
	./configure --prefix=/usr CFLAGS="-O3 -fomit-frame-pointer"
	$BUILD
	make install DESTDIR=${out}
	if [ $? != 0 ]; then echo "Error occured during build" && exit 1; fi
	strip_files "${out}"
	popd
	dpkg_build files/ atv-remoteclient-osmc.deb
	build_return=$?
fi
teardown_env "${1}"
exit $build_return
