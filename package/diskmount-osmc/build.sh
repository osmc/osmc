# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

echo -e "Building disk mounting package"
make clean
dpkg -b files/ diskmount-osmc.deb
