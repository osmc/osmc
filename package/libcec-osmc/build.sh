# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

echo -e "Building libcec"
out=$(pwd)/files
make clean
handle_dep "libusb-dev"
handle_dep "libudev-dev"
sed '/Package/d' -i files/DEBIAN/control
sed '/Package/d' -i files-dev/DEBIAN/control
sed '/Depends/d' -i files-dev/DEBIAN/control
test "$1" == gen && echo "Package: libcec-osmc" >> files/DEBIAN/control && echo "Package: libcec-dev-osmc" >> files-dev/DEBIAN/control && echo "Depends: libcec-osmc" >> files-dev/DEBIAN/control
test "$1" == rbp && echo "Package: rbp-libcec-osmc" >> files/DEBIAN/control && echo "Package: rbp-libcec-dev-osmc" >> files-dev/DEBIAN/control && echo "Depends: rbp-libcec-osmc" >> files-dev/DEBIAN/control
echo Downloading sources
pull_source "https://github.com/Pulse-Eight/libcec" "$(pwd)/src"
pushd src
git checkout release
test "$1" == rbp && install_patch "../patches" "rbp"
./bootstrap
test "$1" == gen && ./configure --prefix=/usr
test "$1" == rbp && ./configure --prefix=/usr --enable-rpi --with-rpi-include-path=/opt/vc/include --with-rpi-lib-path=/opt/vc/lib
$BUILD
make install DESTDIR=${out}
if [ $? != 0 ]; then echo "Error occured during build" && exit 1; fi
strip_files "${out}"
popd
mkdir -p files-dev/usr
mv files/usr/include  files-dev/usr/
fix_arch_ctl "files/DEBIAN/control"
fix_arch_ctl "files-dev/DEBIAN/control"
dpkg -b files/ libcec-osmc.deb
dpkg -b files-dev libcec-dev-osmc.deb
