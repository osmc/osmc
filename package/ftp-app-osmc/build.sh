# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

echo -e "Building package ftp-app-osmc"
publish_applications_any $(pwd) "ftp-app-osmc"
dpkg_build files/ ftp-app-osmc.deb
