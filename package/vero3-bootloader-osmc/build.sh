# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

echo -e "Building vero3-bootloader-osmc"
out=$(pwd)/files
make clean
dpkg_build files/ vero3-bootloader-osmc.deb
