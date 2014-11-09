# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

echo -e "Building libafpclient"
out=$(pwd)/files
make clean
sed '/Package/d' -i files/DEBIAN/control
sed '/Package/d' -i files-dev/DEBIAN/control
sed '/Depends/d' -i files-dev/DEBIAN/control
test "$1" == gen && echo "Package: libafpclient-osmc" >> files/DEBIAN/control && echo "Package: libafpclient-dev-osmc" >> files-dev/DEBIAN/control && echo "Depends: libafpclient-osmc" >> files-dev/DEBIAN/control
test "$1" == rbp && echo "Package: rbp-libafpclient-osmc" >> files/DEBIAN/control && echo "Package: rbp-libafpclient-dev-osmc" >> files-dev/DEBIAN/control && echo "Depends: rbp-libafpclient-osmc" >> files-dev/DEBIAN/control
pull_source "https://github.com/Sky-git/afpfs-ng-fork" "$(pwd)/src"
cd src/afpfs-ng
chmod +x configure
./configure --prefix=/usr
$BUILD
make install DESTDIR=${out}
# We always error
#if [ $? != 0 ]; then echo "Error occured during build" && exit 1; fi
strip_files "${out}"
cd ../../
mkdir -p files-dev/usr
mv files/usr/include  files-dev/usr/
fix_arch_ctl "files/DEBIAN/control"
fix_arch_ctl "files-dev/DEBIAN/control"
dpkg -b files/ libafpclient-osmc.deb
dpkg -b files-dev libafpclient-dev-osmc.deb
