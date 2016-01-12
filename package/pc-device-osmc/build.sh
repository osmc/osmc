# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

echo -e "Building package pc-device-osmc"
make clean
dpkg_build files/ pc-device-osmc.deb
