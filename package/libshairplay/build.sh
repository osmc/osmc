# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

echo -e "Building libshairplay"
out=$(pwd)/files
if [ -d files/usr ]; then rm -rf files/usr; fi
if [ -d files-dev/usr ]; then rm -rf files-dev/usr; fi
cd src
make clean
./autogen.sh
./configure --prefix=/usr
sed '/Package/d' -i files/DEBIAN/control
sed '/Package/d' -i files-dev/DEBIAN/control
test $1 == gen && echo "Package: libshairplay-osmc" >> files/DEBIAN/control && echo "Package: libshairplaydev-osmc" >> files-dev/DEBIAN/control
test $1 == rbp && echo "Package: rbp-libshairplay-osmc" >> files/DEBIAN/control && echo "Package: rbp-libshairplaydev-osmc" >> files-dev/DEBIAN/control
make
make install DESTDIR=${out}
if [ $? != 0 ]; then echo "Error occured during build" && exit 1; fi
cd ../
mkdir -p files-dev/usr
mv files/usr/include  files-dev/usr/
fix_arch_ctl "files/DEBIAN/control"
fix_arch_ctl "files-dev/DEBIAN/control"
dpkg -b files/ libshairplay.deb
dpkg -b files-dev libshairplay-dev.deb
