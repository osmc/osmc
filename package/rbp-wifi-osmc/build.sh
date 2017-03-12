# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

echo -e "Building package rbp-wifi-osmc"
make clean
dpkg_build files/ rbp-wifi-osmc.deb
