# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

echo -e "Building package vero5-earlysplash-osmc"
dpkg_build files/ vero5-earlysplash-osmc.deb
