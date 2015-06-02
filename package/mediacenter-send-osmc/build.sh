# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

echo -e "Building mediacenter-send"
make clean
dpkg_build files/ mediacenter-send-osmc.deb
