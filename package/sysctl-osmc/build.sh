# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

echo -e "Building sysctl tweaks"
make clean
dpkg -b files/ sysctl-osmc.deb
