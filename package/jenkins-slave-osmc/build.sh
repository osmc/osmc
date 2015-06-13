# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

echo -e "Building jenkins-slave package"
make clean
dpkg_build files/ jenkins-slave-osmc.deb
