# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

REV="a29835c79a5b83997dfe3894130a972c67d41171"

echo -e "Building package vero5-userland"
out=$(pwd)/files
sed '/Depends/d' -i files-dev/DEBIAN/control
sed '/Version/d' -i files-dev/DEBIAN/control
VERSION_DEV=$(grep Version ${out}/DEBIAN/control)
VERSION_NUM=$(echo $VERSION_DEV | awk {'print $2'})
echo $VERSION_DEV >> files-dev/DEBIAN/control
echo "Depends: vero5-userland-osmc (=${VERSION_NUM})" >> files-dev/DEBIAN/control
make clean
echo Downloading OpenGL userland
pull_source "https://github.com/osmc/vero5-opengl/archive/${REV}.tar.gz" "$(pwd)/src"
if [ $? != 0 ]; then echo -e "Error downloading" && exit 1; fi
echo Moving files in to place
mkdir -p files/opt/vero5
mkdir -p files-dev/opt/vero5
mkdir -p files/etc/ld.so.conf.d
cp -ar src/vero5-opengl-*/opt/vero5/lib files/opt/vero5
cp -ar src/vero5-opengl-*/opt/vero5/include files-dev/opt/vero5
dpkg_build files/ vero5-userland-osmc.deb
dpkg_build files-dev vero5-userland-dev-osmc.deb
