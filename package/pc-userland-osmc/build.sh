# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

echo -e "Building package pc-userland"
out=$(pwd)/files
make clean
dpkg_build files/ pc-userland-osmc.deb
