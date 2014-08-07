# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

echo -e "Building sysctl tweaks"
make clean
dpkg -b files/ osmc-sysctl.deb
