# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

REV="eeee911dbd53819578a40cf94655305f1979c232"
echo -e "Building package dvb-firmware-osmc"
out=$(pwd)/files
make clean
echo Downloading firmware
pull_source "https://github.com/osmc/dvb-firmware-osmc/archive/${REV}.tar.gz" "$(pwd)/src"
if [ $? != 0 ]; then echo -e "Error downloading" && exit 1; fi
echo Moving files in to place
mkdir -p files/lib/firmware
cp -ar src/dvb-firmware-osmc-${REV}/* files/lib/firmware
dpkg_build files/ dvb-firmware-osmc.deb
