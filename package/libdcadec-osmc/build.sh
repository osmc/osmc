# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

pull_source "https://github.com/foo86/dcadec/archive/ee687982dc1fe453513a46370f97913d729154e4.tar.gz" "$(pwd)/src"
if [ $? != 0 ]; then echo -e "Error downloading" && exit 1; fi
# Build in native environment
build_in_env "${1}" $(pwd) "libdcadec-osmc"
build_return=$?
if [ $build_return == 99 ]
then
	echo -e "Building libdcadec"
	out=$(pwd)/files
	make clean
	sed '/Package/d' -i files/DEBIAN/control
	sed '/Package/d' -i files-dev/DEBIAN/control
	sed '/Depends/d' -i files-dev/DEBIAN/control
        sed '/Version/d' -i files-dev/DEBIAN/control
        VERSION_DEV=$(grep Version ${out}/DEBIAN/control)
        VERSION_NUM=$(echo $VERSION_DEV | awk {'print $2'})
        echo $VERSION_DEV >> files-dev/DEBIAN/control
        echo "Depends: ${1}-libdcadec-osmc (=${VERSION_NUM})" >> files-dev/DEBIAN/control
	echo "Package: ${1}-libdcadec-osmc" >> files/DEBIAN/control && echo "Package: ${1}-libdcadec-dev-osmc" >> files-dev/DEBIAN/control
	pushd src/dcadec*
	install_patch "../../patches" "all"
	$BUILD
	if [ $? != 0 ]; then echo "Error occured during build" && exit 1; fi
	make install DESTDIR=$out
        strip_files "${out}"
	popd
        mkdir -p files-dev/usr
        mv files/usr/include files-dev/usr/
	fix_arch_ctl "files/DEBIAN/control"
	dpkg_build files/ ${1}-libdcadec-osmc.deb
	fix_arch_ctl "files-dev/DEBIAN/control"
	dpkg_build files-dev ${1}-libdcadec-dev-osmc.deb
	build_return=$?
fi
teardown_env "${1}"
exit $build_return
