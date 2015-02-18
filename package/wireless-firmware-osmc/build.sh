# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

echo -e "Building package WiFi firmware"

dpkg -b files/ wireless-firmware-osmc.deb
