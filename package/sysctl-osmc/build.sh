# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

echo -e "Building sysctl tweaks"
make clean
dpkg_build files/ sysctl-osmc.deb
