# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

echo -e "Building package mediacenter-skin-osmc"
dpkg_build files/ mediacenter-skin-osmc.deb
