# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

echo -e "Building libafpclient"
out=$(pwd)/files
if [ -f files/usr ]; then rm -rf files/usr; fi
if [ -f files-dev/usr ]; then rm -rf files-dev/usr; fi
cd src
make clean
./configure
make
make install DESTDIR=${out}
cd ../
mkdir -p files-dev/usr
rm -rf files/usr/bin
rm -rf files/usr/share
mv files/usr/include  files-dev/usr/
fix_arch_ctl "files/DEBIAN/control"
fix_arch_ctl "files-dev/DEBIAN/control"
dpkg -b files/ libafpclient.deb
dpkg -b files-dev libafpclient-dev.deb
