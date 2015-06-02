# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

echo -e "Building package smb-app-osmc"
publish_applications_any $(pwd) "smb-app-osmc"
dpkg_build files/ smb-app-osmc.deb
