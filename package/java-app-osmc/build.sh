# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

echo -e "Building package java-app-osmc"
publish_applications_any $(pwd) "java-app-osmc"
dpkg_build files/ java-app-osmc.deb
