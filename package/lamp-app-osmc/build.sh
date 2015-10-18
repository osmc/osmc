# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

echo -e "Building package lamp-app-osmc"
publish_applications_any $(pwd) "lamp-app-osmc"
dpkg_build files/ lamp-app-osmc.deb
