# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

make clean
pull_source "https://raw.githubusercontent.com/ruuk/repository/master/beta/service.xbmc.tts/service.xbmc.tts-1.0.6b1.zip" "$(pwd)/files/usr/share/kodi/addons"
if [ $? != 0 ]; then echo -e "Error downloading" && exit 1; fi
echo -e "Building package mediacenter-screenreader-osmc"
dpkg_build files/ mediacenter-screenreader-osmc.deb
