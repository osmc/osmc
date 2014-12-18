# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

echo -e "Building mediacenter-eventclients-common"
make clean
dpkg -b files/ mediacenter-eventclients-common-osmc.deb
