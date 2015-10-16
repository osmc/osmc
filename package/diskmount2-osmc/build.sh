# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

build_in_env "${1}" $(pwd) "diskmount2-osmc"
build_return=$?
if [ $build_return == 99 ]
then
        echo -e "Building diskmount2-osmc"
        make clean
        sed '/Package/d' -i files/DEBIAN/control
        sed '/Depends/d' -i files/DEBIAN/control
        echo "Package: ${1}-diskmount2-osmc" >> files/DEBIAN/control
        echo "Depends: ${1}-udevil-osmc, dosfstools, exfat-utils, exfat-fuse, hfsutils, ntfs-3g, policykit-1, eject, ${1}-hd-idle-osmc" >> files/DEBIAN/control
        fix_arch_ctl "files/DEBIAN/control"
        pushd files
        popd
        dpkg_build files/ ${1}-diskmount2.deb
        build_return=$?
fi
teardown_env "${1}"
exit $build_return
