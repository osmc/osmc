# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

echo -e "Building package libamcodec-osmc"
out=$(pwd)/files
make clean
sed '/Depends/d' -i files-dev/DEBIAN/control
sed '/Version/d' -i files-dev/DEBIAN/control
VERSION_DEV=$(grep Version ${out}/DEBIAN/control)
VERSION_NUM=$(echo $VERSION_DEV | awk {'print $2'})
echo $VERSION_DEV >> files-dev/DEBIAN/control
echo "Depends: libamcodec-osmc (=${VERSION_NUM})" >> files-dev/DEBIAN/control
dpkg_build files/ libamcodec-osmc.deb
dpkg_build files-dev libamcodec-dev-osmc.deb
