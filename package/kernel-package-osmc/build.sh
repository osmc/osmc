# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

REV="2855919f3acd83f4ea416d90b83f5ef3299abf3c"

echo -e "Building package kernel-package-osmc"
out=$(pwd)/files
make clean
echo Downloading source
pull_source "https://github.com/osmc/kernel-package-tool-osmc/archive/${REV}.tar.gz" "$(pwd)/src"
if [ $? != 0 ]; then echo -e "Error downloading" && exit 1; fi
dpkg_build files/ kernel-package-osmc.deb
