# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

echo -e "Building disk mounting package"
make clean
dpkg -b files/ diskmount-osmc.deb
