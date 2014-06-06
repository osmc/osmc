# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

echo -e "Building package Darwin-X for AppleTV"
dpkg -b files/ osmc-appletv-darwinx.deb
