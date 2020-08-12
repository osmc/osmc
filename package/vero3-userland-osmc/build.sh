# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

REV="95165155b3acae359c161452b6256e1a5e620325"

echo -e "Building package vero3-userland"
out=$(pwd)/files
sed '/Depends/d' -i files-dev/DEBIAN/control
sed '/Version/d' -i files-dev/DEBIAN/control
VERSION_DEV=$(grep Version ${out}/DEBIAN/control)
VERSION_NUM=$(echo $VERSION_DEV | awk {'print $2'})
echo $VERSION_DEV >> files-dev/DEBIAN/control
echo "Depends: vero3-userland-osmc (=${VERSION_NUM})" >> files-dev/DEBIAN/control
make clean
echo Downloading OpenGL userland
pull_source "https://github.com/osmc/vero3-opengl/archive/${REV}.tar.gz" "$(pwd)/src"
if [ $? != 0 ]; then echo -e "Error downloading" && exit 1; fi
echo Moving files in to place
mkdir -p files/opt/vero3
mkdir -p files-dev/opt/vero3
mkdir -p files/etc/ld.so.conf.d
cp -ar src/vero3-opengl-*/opt/vero3/lib files/opt/vero3
cp -ar src/vero3-opengl-*/opt/vero3/include files-dev/opt/vero3
dpkg_build files/ vero3-userland-osmc.deb
dpkg_build files-dev vero3-userland-dev-osmc.deb
