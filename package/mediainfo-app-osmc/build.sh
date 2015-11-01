# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

echo -e "Building package mediainfo-app-osmc"
publish_applications_any $(pwd) "mediainfo-app-osmc"
dpkg_build files/ mediainfo-app-osmc.deb
