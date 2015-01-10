# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

echo -e "Building package rbp-userland"
out=$(pwd)/files
make clean
echo Downloading userland
pull_source "https://github.com/raspberrypi/firmware" "$(pwd)/firmware-master"
if [ $? != 0 ]; then echo -e "Error downloading" && exit 1; fi
echo Moving files in to place
mkdir -p files/opt/vc
mkdir -p files-dev/opt/vc
mkdir -p files/etc/ld.so.conf.d
echo "/opt/vc/lib" > files/etc/ld.so.conf.d/rbp.conf
cp -ar firmware-master/hardfp/opt/vc/bin/ files/opt/vc
cp -ar firmware-master/hardfp/opt/vc/lib files/opt/vc
cp -ar firmware-master/hardfp/opt/vc/include files-dev/opt/vc
rm -rf firmware-master
dpkg -b files/ rbp-userland-osmc.deb
dpkg -b files-dev/ rbp-userland-dev-osmc.deb
