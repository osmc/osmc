# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh
pull_source "https://github.com/osmc/evrepeat/archive/5b46d7299e514e74ce48b9bc40f63c156e3fae58.tar.gz" "$(pwd)/src"
if [ $? != 0 ]; then echo -e "Error downloading" && exit 1; fi
# Build in native environment
build_in_env "${1}" $(pwd) "evrepeat-osmc"
build_return=$?
if [ $build_return == 99 ]
then
	echo -e "Building evrepeat-osmc"
	out=$(pwd)/files
	make clean
	sed '/Package/d' -i files/DEBIAN/control
	update_sources
	echo "Package: ${1}-evrepeat-osmc" >> files/DEBIAN/control
	pushd src/evrepeat-*
	$BUILD
	make install DESTDIR=${out}
	if [ $? != 0 ]; then echo "Error occured during build" && exit 1; fi
	strip_files "${out}"
	popd
	fix_arch_ctl "files/DEBIAN/control"
	dpkg_build files/ ${1}-evrepeat-osmc.deb
	build_return=$?
fi
teardown_env "${1}"
exit $build_return
