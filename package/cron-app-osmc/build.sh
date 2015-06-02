# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

echo -e "Building package cron-app-osmc"
publish_applications_any $(pwd) "cron-app-osmc"
dpkg_build files/ cron-app-osmc.deb
