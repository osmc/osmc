# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

echo -e "Building ftr"
make clean
dpkg -b files/ osmc-ftr.deb
