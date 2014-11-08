# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

echo -e "Building mediacenter-send"
make clean
dpkg -b files/ mediacenter-send-osmc.deb
