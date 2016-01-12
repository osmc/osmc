# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

echo -e "Building package atv-bootloader"
pull_source "https://github.com/osmc/atv-bootloader/archive/0438fe3769d02cce486d8dbb8d3ba0ae8540bfda.tar.gz" "$(pwd)/src/"
out=$(pwd)/files
make clean
mkdir -p ${out}/opt/atvbl
mkdir -p ${out}/opt/darwin-cross
mv src/atv-bootloader*/darwin-cross ${out}/opt
mv src/atv-bootloader*/* ${out}/opt/atvbl
sed -e s:\$\(pwd\):/opt: -i files/opt/atvbl/Makefile
dpkg_build files/ atv-bootloader-osmc.deb
