# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

echo -e "Building package ssh-app-osmc"
publish_applications_any $(pwd)
dpkg -b files/ ssh-app-osmc.deb
