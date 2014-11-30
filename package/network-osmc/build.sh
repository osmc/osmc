# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

echo -e "Building networking package"
make clean
dpkg -b files/ network-osmc.deb
