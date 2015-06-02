# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

echo -e "Building package rbp1-device-osmc"
make clean
dpkg_build files/ rbp1-device-osmc.deb
