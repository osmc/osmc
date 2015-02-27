# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

# Build in native environment
pull_source "https://github.com/samnazarko/atvclient/archive/master.zip" "$(pwd)/src"
build_in_env "${1}" $(pwd) "atv-remoteclient-osmc"
if [ $? == 0 ]
then
	echo -e "Building atvclient"
	out=$(pwd)/files
	make clean
	update_sources
	handle_dep "libusb-dev"
	pushd src/
	./configure --prefix=/usr
	$BUILD
	make install DESTDIR=${out}
	if [ $? != 0 ]; then echo "Error occured during build" && exit 1; fi
	strip_files "${out}"
	popd
	dpkg -b files/ atv-remoteclient-osmc.deb
fi
teardown_env "${1}"
