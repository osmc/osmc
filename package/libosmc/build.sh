# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

echo -e "Building libosmc"
out=$(pwd)/files
make clean
sed '/Package/d' -i files/DEBIAN/control
test "$1" == gen && echo "Package: libosmc" >> files/DEBIAN/control
test "$1" == rbp && echo "Package: rbp-libosmc" >> files/DEBIAN/control
cd src
make clean
$BUILD
if [ $? != 0 ]; then echo "Error occured during build" && exit 1; fi
cd ../
mkdir -p files/usr/lib
cp src/libosmc.so files/usr/lib
fix_arch_ctl "files/DEBIAN/control"
dpkg -b files/ osmc-libosmc.deb
