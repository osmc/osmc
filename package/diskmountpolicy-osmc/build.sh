# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

echo -e "Building OSMC policy for udisks"
make clean
dpkg -b files/ osmc-diskmountpolicy.deb
