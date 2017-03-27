# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

pull_source "https://github.com/miloyip/rapidjson/archive/v1.1.0.tar.gz" "$(pwd)/src"
if [ $? != 0 ]; then echo -e "Error downloading" && exit 1; fi
mkdir -p files-dev/usr/include
cp -ar src/*rapidjson*/include files-dev/usr/include
dpkg_build files-dev rapidjson-dev-osmc.deb
