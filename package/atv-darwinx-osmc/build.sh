# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

echo -e "Building package Darwin-X for AppleTV"
make clean
dpkg -b files/ appletv-darwinx-osmc.deb
