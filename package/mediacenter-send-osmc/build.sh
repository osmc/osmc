# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

echo -e "Building mediacenter-send"
make clean
dpkg -b files/ mediacenter-send-osmc.deb
