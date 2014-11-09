# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

echo -e "Building libshairplay"
out=$(pwd)/files
make clean
sed '/Package/d' -i files/DEBIAN/control
sed '/Package/d' -i files-dev/DEBIAN/control
sed '/Depends/d' -i files-dev/DEBIAN/control
test $1 == gen && echo "Package: libshairplay-osmc" >> files/DEBIAN/control && echo "Package: libshairplay-dev-osmc" >> files-dev/DEBIAN/control && echo "Depends: libshairplay-osmc" >> files-dev/DEBIAN/control
test $1 == rbp && echo "Package: rbp-libshairplay-osmc" >> files/DEBIAN/control && echo "Package: rbp-libshairplay-dev-osmc" >> files-dev/DEBIAN/control && echo "Depends: rbp-libshairplay-osmc" >> files-dev/DEBIAN/control
pull_source "http://psg.mtu.edu/pub/xbmc/build-deps/sources/shairplay-139d5ef.tar.bz2" "$(pwd)/src"
cd src/shairplay*
./autogen.sh
./configure --prefix=/usr
$BUILD
make install DESTDIR=${out}
if [ $? != 0 ]; then echo "Error occured during build" && exit 1; fi
strip_files "${out}"
cd ../../
mkdir -p files-dev/usr
mv files/usr/include  files-dev/usr/
fix_arch_ctl "files/DEBIAN/control"
fix_arch_ctl "files-dev/DEBIAN/control"
dpkg -b files/ libshairplay-osmc.deb
dpkg -b files-dev libshairplay-dev-osmc.deb
