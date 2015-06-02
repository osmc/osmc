# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

echo -e "Building performancetuner"
make clean
dpkg_build files/ perftune-osmc.deb
