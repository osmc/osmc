# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

echo -e "Building package vero-device-osmc"
make clean
dpkg -b files/ vero-device-osmc.deb
