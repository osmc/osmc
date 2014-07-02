# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

echo -e "Building librtmp"
out=$(pwd)/files
if [ -d files/usr/ ]; then rm -rf files/usr; fi
if [ -d files-dev/usr ]; then rm -rf files-dev/usr; fi
cd src
make clean
make sys=posix
if [ $? != 0 ]; then echo "Error occured during build" && exit 1; fi
cd librtmp
strip_libs
mkdir -p $out/usr/lib
cp -ar librtmp.so.* $out/usr/lib
cp -ar librtmp.a $out/usr/lib
cd ../../
fix_arch_ctl "files/DEBIAN/control"
dpkg -b files/ librtmp-osmc.deb
out=$(pwd)/files-dev
cd src
cd librtmp
mkdir -p $out/usr/include/librtmp
cp -ar *.h $out/usr/include/librtmp
cd ../../
fix_arch_ctl "files-dev/DEBIAN/control"
dpkg -b files-dev librtmpdev-osmc.deb
