# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

REV="526ac57919efa39729203c02d82f2bc666d18b3f"

echo -e "Building package vero2-userland"
out=$(pwd)/files
sed '/Depends/d' -i files-dev/DEBIAN/control
sed '/Version/d' -i files-dev/DEBIAN/control
VERSION_DEV=$(grep Version ${out}/DEBIAN/control)
VERSION_NUM=$(echo $VERSION_DEV | awk {'print $2'})
echo $VERSION_DEV >> files-dev/DEBIAN/control
echo "Depends: vero2-userland-osmc (=${VERSION_NUM})" >> files-dev/DEBIAN/control
make clean
echo Downloading OpenGL userland
pull_source "https://github.com/osmc/vero2-opengl/archive/${REV}.tar.gz" "$(pwd)/src"
if [ $? != 0 ]; then echo -e "Error downloading" && exit 1; fi
echo Moving files in to place
mkdir -p files/opt/vero2
mkdir -p files-dev/opt/vero2
mkdir -p files/etc/ld.so.conf.d
echo "/opt/vero2/lib" > files/etc/ld.so.conf.d/vero2.conf
cp -ar src/vero2-opengl-${REV}/opt/vero2/lib files/opt/vero2/
cp -ar src/vero2-opengl-${REV}/opt/vero2/include files-dev/opt/vero2
dpkg_build files/ vero2-userland-osmc.deb
dpkg_build files-dev vero2-userland-dev-osmc.deb
