# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

echo -e "Building vero3-wifi-osmc"
out=$(pwd)/files
make clean
dpkg_build files/ vero3-wifi-osmc.deb
