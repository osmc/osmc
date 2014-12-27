# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

pull_source "https://github.com/samnazarko/osmc-kodi-addons" "$(pwd)/src"
echo -e "Building package mediacenter-addon-osmc"
make clean
rm src/README.md
rm -rf src/script.module.osmcsetting.template*
mkdir -p files/usr/share/kodi/addons
cp -ar src/* files/usr/share/kodi/addons
dpkg -b files/ mediacenter-addon-osmc.deb
teardown_env "${1}"
