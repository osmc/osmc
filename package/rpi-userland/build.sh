# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

echo -e "Building rpi-userland"
out=$(pwd)/files
if [ -d files/opt ]; then rm -rf files/opt; fi
if [ -d files-dev/opt ]; then rm -rf files-dev/opt; fi
if [ -d firmware-master ]; then rm -rf firmware-master; fi
if [ -f master.zip ]; then rm master.zip; fi
echo Downloading userland
wget --no-check-certificate "https://github.com/raspberrypi/firmware/archive/master.zip"
if [ $? != 0 ]; then echo -e "Error downloading" && exit 1; fi
unzip master.zip
rm master.zip
echo Moving files in to place
mkdir -p files/opt/vc
mkdir -p files-dev/opt/vc
cp -ar firmware-master/hardfp/opt/vc/bin/ files/opt/vc
cp -ar firmware-master/hardfp/opt/vc/lib files/opt/vc
cp -ar firmware-master/hardfp/opt/vc/include files-dev/opt/vc
rm -rf firmware-master
dpkg -b files/ rpiuserland.deb
dpkg -b files-dev/ rpiuserland-dev.deb
