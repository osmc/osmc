# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

REV="cd00af6d392d1b8bccc20c431da94339f9e99045"
pull_source "https://github.com/osmc/vero5-wifi-osmc" "$(pwd)/src" "$REV"
if [ $? != 0 ]; then echo -e "Error fetching vero5-wifi-osmc source" && exit 1; fi
# Build in native environment
build_in_env "${1}" $(pwd) "vero5-wifi-osmc"
build_return=$?
if [ $build_return == 99 ]
then
        echo -e "Building package vero5-wifi-osmc"
        out=$(pwd)/files
        make clean
        update_sources
        sed '/Package/d' -i files/DEBIAN/control
        echo "Package: ${1}-wifi-osmc" >> files/DEBIAN/control
        pushd src/
        make
        if [ $? != 0 ]; then echo -e "Build failed!" && exit 1; fi
        mkdir -p ${out}/usr/bin
	cp rtk_hciattach ${out}/usr/bin
	chmod +x ${out}/usr/bin/rtk_hciattach
        popd
        strip_files "${out}"
        fix_arch_ctl "files/DEBIAN/control"
        dpkg_build files/ vero5-wifi-osmc.deb
        build_return=$?
fi
teardown_env "${1}"
exit $build_return
