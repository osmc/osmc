# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

echo -e "Building OSMC policy to allow NICE adjustment"
make clean
dpkg -b files/ niceprioritypolicy-osmc.deb
