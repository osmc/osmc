# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

echo -e "Building package mediacenter-addon-osmc"
make clean
mkdir -p files/usr/share/kodi/addons
cp -ar src/* files/usr/share/kodi/addons
rm -rf files/usr/share/kodi/addons/script.module.osmcsetting.template*
dpkg_build files/ mediacenter-addon-osmc.deb
