# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

pull_source "git://git.ffmpeg.org/rtmpdump" "$(pwd)/src"
if [ $? != 0 ]; then echo -e "Error downloading" && exit 1; fi
# Build in native environment
build_in_env "${1}" $(pwd) "librtmp-osmc"
build_return=$?
if [ $build_return == 99 ]
then
	echo -e "Building librtmp"
	out=$(pwd)/files
	make clean
	sed '/Package/d' -i files/DEBIAN/control
	sed '/Package/d' -i files-dev/DEBIAN/control
	sed '/Depends/d' -i files-dev/DEBIAN/control
        sed '/Version/d' -i files-dev/DEBIAN/control
        VERSION_DEV=$(grep Version ${out}/DEBIAN/control)
        VERSION_NUM=$(echo $VERSION_DEV | awk {'print $2'})
        echo $VERSION_DEV >> files-dev/DEBIAN/control
        echo "Depends: ${1}-librtmp-osmc (=${VERSION_NUM})" >> files-dev/DEBIAN/control
	update_sources
	handle_dep "libssl-dev"
	echo "Package: ${1}-librtmp-osmc" >> files/DEBIAN/control && echo "Package: ${1}-librtmp-dev-osmc" >> files-dev/DEBIAN/control
	pushd src
	install_patch "../patches" "all"
	$BUILD sys=posix
	if [ $? != 0 ]; then echo "Error occured during build" && exit 1; fi
	pushd librtmp
	strip_libs
	mkdir -p $out/usr/lib
	cp -ar librtmp.so.1 $out/usr/lib
	cp -ar librtmp.a $out/usr/lib
	pushd $out/usr/lib
	ln -s librtmp.so.1 librtmp.so
	popd
	popd
	popd
	fix_arch_ctl "files/DEBIAN/control"
	dpkg_build files/ ${1}-librtmp-osmc.deb
	out=$(pwd)/files-dev
	cd src
	cd librtmp
	mkdir -p $out/usr/include/librtmp
	cp -ar *.h $out/usr/include/librtmp
	cd ../../
	fix_arch_ctl "files-dev/DEBIAN/control"
	dpkg_build files-dev ${1}-librtmp-dev-osmc.deb
	build_return=$?
fi
teardown_env "${1}"
exit $build_return
