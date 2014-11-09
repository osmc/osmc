# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

echo -e "Building package ssh-app-osmc"
dpkg -b files/ ssh-app-osmc.deb
