# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

echo -e "Building package WiFi firmware"

dpkg_build files/ wireless-firmware-osmc.deb
