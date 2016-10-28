# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

echo -e "Building package vero3-earlysplash-osmc"
dpkg_build files/ vero3-earlysplash-osmc.deb
