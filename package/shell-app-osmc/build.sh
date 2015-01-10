# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

echo -e "Building package shell-app-osmc"
dpkg -b files/ shell-app-osmc.deb
