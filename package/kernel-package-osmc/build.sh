# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

REV="c53ec1332c4db864a1f626d2fe4a5ac9fa5c1f6a"

echo -e "Building package kernel-package-osmc"
out=$(pwd)/files
make clean
echo Downloading source
pull_source "https://github.com/osmc/kernel-package-tool-osmc/archive/${REV}.tar.gz" "$(pwd)/src"
if [ $? != 0 ]; then echo -e "Error downloading" && exit 1; fi
pushd src/kernel-package-tool-osmc-${REV}
make install DESTDIR=${out}
popd
dpkg_build files/ kernel-package-osmc.deb
