# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

echo -e "Building package vero5-secureosmc-osmc"
make clean
dpkg_build files/ vero5-secureosmc-osmc.deb
