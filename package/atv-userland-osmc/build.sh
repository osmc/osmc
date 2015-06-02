# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

echo -e "Building package atv-userland"
out=$(pwd)/files
make clean
dpkg_build files/ atv-userland-osmc.deb
