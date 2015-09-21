# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

REV="393e8bcffd6e073b94d4c002d08edb5f9dfcea78"

echo -e "Building package rbp-userland"
out=$(pwd)/files
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
rm -rf src
dpkg_build files/ rbp-userland-osmc.deb
dpkg_build files-dev/ rbp-userland-dev-osmc.deb
dpkg_build files-src/ rbp-userland-src-osmc.deb
