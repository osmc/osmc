# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

echo -e "Building disk mounting package"
make clean
dpkg_build files/ diskmount-osmc.deb
