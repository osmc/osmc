# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

if [ "$1" == "trans" ]
then
    echo -e "Building transitional package"
    dpkg_build files-trans ${1}-diskmount-osmc.deb
    exit 0
fi

echo -e "Building disk mounting package"
# Build in native environment
build_in_env "${1}" $(pwd) "diskmount-osmc"
build_return=$?
if [ $build_return == 99 ]
then
    sed '/^Package/d' -i files/DEBIAN/control
    echo "Package: ${1}-diskmount-osmc" >> files/DEBIAN/control
    make clean
    sed '/^Depends/d' -i files/DEBIAN/control
    echo "Depends: ${1}-udisks-osmc, ${1}-udisks-glue-osmc, hdparm, dosfstools, exfat-utils, exfat-fuse, hfsutils, hfsprogs, ntfs-3g, policykit-1, eject" >> files/DEBIAN/control
    fix_arch_ctl "files/DEBIAN/control"
    dpkg_build files/ ${1}-diskmount-osmc.deb
    build_return=$?
fi
teardown_env "${1}"
exit $build_return

