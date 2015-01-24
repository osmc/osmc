# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

echo -e "Building package Darwin Cross for AppleTV"
make clean
dpkg -b files/ atv-darwin-cross-osmc.deb
