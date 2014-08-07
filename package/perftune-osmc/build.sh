# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

echo -e "Building performancetuner"
make clean
dpkg -b files/ osmc-perftune.deb
