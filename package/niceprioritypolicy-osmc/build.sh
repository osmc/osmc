# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

echo -e "Building OSMC policy to allow NICE adjustment"
make clean
dpkg_build files/ niceprioritypolicy-osmc.deb
