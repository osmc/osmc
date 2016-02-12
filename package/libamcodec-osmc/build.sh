# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

pull_source "https://github.com/osmc/libamcodec/archive/b47da3006d666dd46d6567250ca541250ead462f.tar.gz" "$(pwd)/src"
if [ $? != 0 ]; then echo -e "Error fetching libamcodec source" && exit 1; fi
# Build in native environment
build_in_env "${1}" $(pwd) "libamcodec-osmc"
build_return=$?
if [ $build_return == 99 ]
then
	echo -e "Building package libamcodec-osmc"
	out=$(pwd)/files
	make clean
        sed '/Package/d' -i files/DEBIAN/control
        sed '/Package/d' -i files-dev/DEBIAN/control
        sed '/Depends/d' -i files-dev/DEBIAN/control
        sed '/Version/d' -i files-dev/DEBIAN/control
        VERSION_DEV=$(grep Version ${out}/DEBIAN/control)
        VERSION_NUM=$(echo $VERSION_DEV | awk {'print $2'})
        echo $VERSION_DEV >> files-dev/DEBIAN/control
        echo "Depends: ${1}-libamcodec-osmc (=${VERSION_NUM})" >> files-dev/DEBIAN/control
	update_sources
	handle_dep "libasound2-dev"
	echo "Package: ${1}-libamcodec-osmc" >> files/DEBIAN/control && echo "Package: ${1}-libamcodec-dev-osmc" >> files-dev/DEBIAN/control >> files-dev/DEBIAN/control
	mkdir -p ${out}/usr/osmc/lib
	mkdir -p files-dev/usr/osmc/include/amcodec/
	pushd src/libamcodec*
	$BUILD -C amavutils PREFIX="/usr/osmc"
	if [ $? != 0 ]; then echo "Error occured building amavutils" && exit 1; fi
	cp -ar amavutils/*.so ${out}/usr/osmc/lib
	$BUILD -C amadec PREFIX="/usr/osmc"
	if [ $? != 0 ]; then echo "Error occured building amadec" && exit 1; fi
	cp -ar amadec/*.so ${out}/usr/osmc/lib
	$BUILD -C amcodec PREFIX="/usr/osmc"
	if [ $? != 0 ]; then echo "Error occured building amcodec" && exit 1; fi
	cp -ar amcodec/libamcodec.so.0.0 ${out}/usr/osmc/lib
	cp -ar amcodec/include/*.h ../../files-dev/usr/osmc/include/amcodec
	ln -sf libamcodec.so.0.0 ${out}/usr/osmc/lib/libamcodec.so
	popd
        fix_arch_ctl "files/DEBIAN/control"
        fix_arch_ctl "files-dev/DEBIAN/control"
	dpkg_build files/ ${1}-libamcodec-osmc.deb
	dpkg_build files-dev ${1}-libamcodec-dev-osmc.deb
	build_return=$?
fi
teardown_env "${1}"
exit $build_return
