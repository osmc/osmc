# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

echo -e "Building rpi-userland"
out=$(pwd)/files
make clean
echo Downloading userland
wget --no-check-certificate "https://github.com/raspberrypi/firmware/archive/master.zip"
if [ $? != 0 ]; then echo -e "Error downloading" && exit 1; fi
unzip master.zip
rm master.zip
echo Moving files in to place
mkdir -p files/opt/vc
mkdir -p files-dev/opt/vc
mkdir -p files/etc/ld.so.conf.d
echo "/opt/vc/lib" > files/etc/ld.so.conf.d/rbp.conf
cp -ar firmware-master/hardfp/opt/vc/bin/ files/opt/vc
cp -ar firmware-master/hardfp/opt/vc/lib files/opt/vc
cp -ar firmware-master/hardfp/opt/vc/include files-dev/opt/vc
rm -rf firmware-master
dpkg -b files/ rpiuserland.deb
dpkg -b files-dev/ rpiuserland-dev.deb
