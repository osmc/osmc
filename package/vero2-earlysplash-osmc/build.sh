# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

echo -e "Building package vero2-earlysplash-osmc"
dpkg_build files/ vero2-earlysplash-osmc.deb
