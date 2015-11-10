# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

REV="393e8bcffd6e073b94d4c002d08edb5f9dfcea78"

echo -e "Building package rbp-userland"
out=$(pwd)/files
sed '/Depends/d' -i files-dev/DEBIAN/control
sed '/Version/d' -i files-dev/DEBIAN/control
VERSION_DEV=$(grep Version ${out}/DEBIAN/control)
VERSION_NUM=$(echo $VERSION_DEV | awk {'print $2'})
echo $VERSION_DEV >> files-dev/DEBIAN/control
echo "Depends: rbp-userland-osmc (=${VERSION_NUM})" >> files-dev/DEBIAN/control
echo $VERSION_DEV >> files-src/DEBIAN/control
echo "Depends: rbp-userland-osmc (=${VERSION_NUM})" >> files-src/DEBIAN/control
make clean
echo Downloading userland
pull_source "https://github.com/raspberrypi/firmware/archive/${REV}.tar.gz" "$(pwd)/src"
if [ $? != 0 ]; then echo -e "Error downloading" && exit 1; fi
echo Moving files in to place
mkdir -p files/opt/vc
mkdir -p files-dev/opt/vc
mkdir -p files-src/opt/vc
mkdir -p files/etc/ld.so.conf.d
echo "/opt/vc/lib" > files/etc/ld.so.conf.d/rbp.conf
cp -ar src/firmware-${REV}/hardfp/opt/vc/bin/ files/opt/vc
cp -ar src/firmware-${REV}/hardfp/opt/vc/lib files/opt/vc
cp -ar src/firmware-${REV}/hardfp/opt/vc/include files-dev/opt/vc
cp -ar src/firmware-${REV}/hardfp/opt/vc/src files-src/opt/vc
dpkg_build files/ rbp-userland-osmc.deb
dpkg_build files-dev/ rbp-userland-dev-osmc.deb
dpkg_build files-src/ rbp-userland-src-osmc.deb
