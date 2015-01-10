# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

echo -e "Building ftr"
make clean
dpkg -b files/ ftr-osmc.deb
