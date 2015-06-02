# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

echo -e "Building package vero1-device-osmc"
make clean
dpkg_build files/ vero1-device-osmc.deb
