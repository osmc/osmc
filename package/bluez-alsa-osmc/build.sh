# (c) 2014-2020 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

echo -e "Building package bluez-alsa-app-osmc"

dpkg_build files/ bluez-alsa-app-osmc.deb
