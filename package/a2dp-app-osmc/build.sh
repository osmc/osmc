# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

echo -e "Building package a2dp-app-osmc"
publish_applications_any $(pwd) "a2dp-app-osmc"
dpkg -b files/ a2dp-app-osmc.deb
