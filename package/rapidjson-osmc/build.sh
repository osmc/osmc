# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

pull_source "https://github.com/miloyip/rapidjson/archive/v1.1.0.tar.gz" "$(pwd)/src"

if [ $? != 0 ]; then echo -e "Error downloading" && exit 1; fi
# Build in native environment
build_in_env "${1}" $(pwd) "rapidjson-osmc"
build_return=$?
if [ $build_return == 99 ]
then
        echo -e "Building rapidjson-osmc"
        out=$(pwd)/files-dev
        if [ -d files/usr ]; then rm -rf files/usr; fi
        if [ -d files-dev/usr ]; then rm -rf files-dev/usr; fi
        sed '/Package/d' -i files-dev/DEBIAN/control
        update_sources
	handle_dep "pkg-config"
	handle_dep "${1}-cmake-osmc"
        echo "Package: ${1}-rapidjson-dev-osmc" >> files-dev/DEBIAN/control
        pushd src/rapidjson-*
	cmake -DCMAKE_INSTALL_PREFIX=/usr/osmc -DCMAKE_INSTALL_LIBDIR=/usr/osmc/lib -DCMAKE_INSTALL_LIBDIR_NOARCH=/usr/osmc/lib .
        $BUILD
        make install DESTDIR=${out}
        if [ $? != 0 ]; then echo "Error occured during build" && exit 1; fi
        strip_files "${out}"
        popd
        fix_arch_ctl "files-dev/DEBIAN/control"
        dpkg_build files-dev ${1}-rapidjson-dev-osmc.deb
        build_return=$?
fi
teardown_env "${1}"
exit $build_return
