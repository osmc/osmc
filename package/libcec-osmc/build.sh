# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

echo -e "Building libcec"
out=$(pwd)/files
if [ -d files/usr ]; then rm -rf files/usr; fi
if [ -d files-dev/usr ]; then rm -rf files-dev/usr; fi
sed '/Package/d' -i files/DEBIAN/control
sed '/Package/d' -i files-dev/DEBIAN/control
sed '/Depends/d' -i files-dev/DEBIAN/control
test "$1" == gen && echo "Package: libcec-osmc" >> files/DEBIAN/control && echo "Package: libcecdev-osmc" >> files-dev/DEBIAN/control && echo "Depends: libcec-osmc" >> files-dev/DEBIAN/control
test "$1" == rbp && echo "Package: rbp-libcec-osmc" >> files/DEBIAN/control && echo "Package: rbp-libcecdev-osmc" >> files-dev/DEBIAN/control && echo "Depends: rbp-libcec-osmc" >> files-dev/DEBIAN/control
cd src
make clean
./bootstrap
test "$1" == gen && ./configure --prefix=/usr
test "$1" == rbp && ./configure --prefix=/usr --enable-rpi --with-rpi-include-path=/opt/vc/include --with-rpi-lib-path=/opt/vc/lib
$BUILD
make install DESTDIR=${out}
if [ $? != 0 ]; then echo "Error occured during build" && exit 1; fi
strip_files "${out}"
cd ../
mkdir -p files-dev/usr
mv files/usr/include  files-dev/usr/
fix_arch_ctl "files/DEBIAN/control"
fix_arch_ctl "files-dev/DEBIAN/control"
dpkg -b files/ libcec.deb
dpkg -b files-dev libcec-dev.deb
