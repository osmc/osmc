# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

pull_source "https://github.com/graeme-hill/crossguid/archive/8f399e8bd4252be9952f3dfa8199924cc8487ca4.tar.gz" "$(pwd)/src"
if [ $? != 0 ]; then echo -e "Error downloading" && exit 1; fi
# Build in native environment
build_in_env "${1}" $(pwd) "libcrossguid-osmc"
build_return=$?
if [ $build_return == 99 ]
then
	echo -e "Building libcrossguid"
	out=$(pwd)/files
	if [ -d files/usr ]; then rm -rf files/usr; fi
	if [ -d files-dev/usr ]; then rm -rf files-dev/usr; fi
	sed '/Package/d' -i files/DEBIAN/control
	sed '/Package/d' -i files-dev/DEBIAN/control
	sed '/Depends/d' -i files-dev/DEBIAN/control
        sed '/Version/d' -i files-dev/DEBIAN/control
        VERSION_DEV=$(grep Version ${out}/DEBIAN/control)
        VERSION_NUM=$(echo $VERSION_DEV | awk {'print $2'})
        echo $VERSION_DEV >> files-dev/DEBIAN/control
        echo "Depends: ${1}-libplatform-osmc (=${VERSION_NUM}), libuuid1" >> files-dev/DEBIAN/control
	update_sources
	handle_dep "uuid-dev"
	echo "Package: ${1}-libcrossguid-osmc" >> files/DEBIAN/control && echo "Package: ${1}-libcrossguid-dev-osmc" >> files-dev/DEBIAN/control
	pushd src/crossguid-*
	g++ -c guid.cpp -o guid.o -Wall -std=c++11 -DGUID_LIBUUID
	ar rvs libcrossguid.a guid.o
	if [ $? != 0 ]; then echo "Error occured during build" && exit 1; fi
	popd
	mkdir -p files/usr/lib
	cp src/crossguid-*/libcrossguid.a files/usr/lib
	mkdir -p files-dev/usr/include
	cp -ar src/crossguid-*/guid.h files-dev/usr/include
	fix_arch_ctl "files/DEBIAN/control"
	fix_arch_ctl "files-dev/DEBIAN/control"
	dpkg_build files ${1}-libcrossguid-osmc.deb
	dpkg_build files-dev ${1}-libcrossguid-dev-osmc.deb
	build_return=$?
fi
teardown_env "${1}"
exit $build_return
